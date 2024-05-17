#include <llvm-c/Core.h>
#include"ast.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <unordered_map> 
#include <cstddef>
#include <cstring>
#include <string>
#include<vector>
using namespace std;

LLVMTypeRef ret_read;
LLVMValueRef func_read;
LLVMTypeRef ret_print;
LLVMValueRef func_print;
LLVMValueRef func;
unordered_map<string, LLVMValueRef> allocs;
LLVMValueRef ret_ref;
LLVMBasicBlockRef ret_bb;

int replace_val_names(astNode* node, unordered_map<string, string> *replace) {
	switch(node->type) {
		case ast_stmt: {
			// calls that are vals must be read(), so there is no param
			return 0;
		}
		case ast_var: {
            string var_name = node->var.name;
			if (replace->count(var_name)) {
				string new_var_name = (*replace)[var_name];
				char *new_var_name_arr = (char *) malloc((new_var_name.length() + 1) * sizeof(char));
				strcpy(new_var_name_arr, new_var_name.c_str());
				free(node->var.name);
				node->var.name = new_var_name_arr;
			}
            return 0;
		}
		case ast_cnst:
            return 0;
		case ast_uexpr:
            return replace_val_names(node->uexpr.expr, replace);
		case ast_bexpr: {
            if (replace_val_names(node->bexpr.lhs, replace) == -1) return -1;
            if (replace_val_names(node->bexpr.rhs, replace) == -1) return -1;
            return 0;
		}
		default: {
            printf("val has incorrect type (not call, var, cnst, uexpr, or bexpr)\n");
			return -1;
		}
	}
}


int replace_stmt_names(astNode* node, unordered_map<string, LLVMValueRef> *allocs, unordered_map<string, string> *replace, LLVMBuilderRef *builder) {
	astStmt stmt = node->stmt; 
	switch(stmt.type) {
		case ast_call: {
            if (stmt.call.param != NULL) return replace_val_names(stmt.call.param, replace);
            return 0;
        }
		case ast_ret:
            return replace_val_names(stmt.ret.expr, replace);
		case ast_asgn:	{
            if (replace_val_names(stmt.asgn.lhs, replace) == -1) return -1;
            if (replace_val_names(stmt.asgn.rhs, replace) == -1) return -1;
            return 0;
        }
		case ast_decl:	{
			astDecl decl = stmt.decl;
			string decl_name = decl.name;
			string new_decl_name = decl.name;
			while (allocs->count(new_decl_name)) {
				new_decl_name.append("_new");
			}
			(*replace)[decl_name] = new_decl_name;
			char* new_decl_name_arr = (char *) malloc((new_decl_name.length() + 1)*sizeof(char));
			strcpy(new_decl_name_arr, new_decl_name.c_str());
			free(decl.name);
			node->stmt.decl.name = new_decl_name_arr;

			// Creating an alloc instruction with alignment
			LLVMValueRef alloc = LLVMBuildAlloca(*builder, LLVMInt32Type(), new_decl_name_arr); 
			LLVMSetAlignment(alloc, 4);
			(*allocs)[new_decl_name] = alloc;

			

            return 0;
        }
		case ast_block: {
            return 0;
        }
		case ast_while: {
            if (replace_val_names(stmt.whilen.cond->rexpr.lhs, replace) == -1) return -1;
            if (replace_val_names(stmt.whilen.cond->rexpr.rhs, replace) == -1) return -1;
            return 0;
		}
		case ast_if: {
            if (replace_val_names(stmt.ifn.cond->rexpr.lhs, replace) == -1) return -1;
            if (replace_val_names(stmt.ifn.cond->rexpr.rhs, replace) == -1) return -1;
            return 0;
		}
        default: {
            printf("stmt has incorrect type\n");
            return -1;
        }
	}
}

int replace_scope_names(astNode* node, unordered_map<string, LLVMValueRef> *allocs, unordered_map<string, string> *replace, LLVMBuilderRef *builder) {
	astStmt stmt = node->stmt;
	if (stmt.type == ast_block) {
		// recurse sub-scopes and then perform replacements
		astBlock block = stmt.block;
		for (int i = 0; i < block.stmt_list->size(); i++) {
			astNode *block_node = (*(block.stmt_list))[i];
			astStmt block_stmt = block_node->stmt;
			switch(block_stmt.type) {
				case ast_if: {
					astIf if_stmt = block_stmt.ifn;
					astNode *if_body = if_stmt.if_body;
					if (replace_scope_names(if_body, allocs, replace, builder) == -1) return -1;
					astNode *else_body = if_stmt.else_body;
					if (else_body != NULL) {
						if (replace_scope_names(else_body, allocs, replace, builder) == -1) return -1;
					}
					break;
				}
				case ast_while: {
					astWhile while_stmt = block_stmt.whilen;
					astNode *body = while_stmt.body;
					if (replace_scope_names(body, allocs, replace, builder) == -1) return -1;
					break;
				}
				case ast_block: {
					if (replace_scope_names(block_node, allocs, replace, builder) == -1) return -1;
					break;
				}
			}
		}
		for (int i = 0; i < block.stmt_list->size(); i++) {
			astNode *block_node = (*(block.stmt_list))[i];
			if (replace_stmt_names(block_node, allocs, replace, builder) == -1) return -1;
		}
		return 0;
	}
	return replace_stmt_names(node, allocs, replace, builder);
}

LLVMValueRef genIRExpr (astNode *node, LLVMBuilderRef builder) {
	switch(node->type) {
		case ast_cnst: {
			// Generate an LLVMValueRef using LLVMConstInt using the constant value in the node.
			LLVMValueRef const_val = LLVMConstInt(LLVMInt32Type(), node->cnst.value, 1);
			// Return the LLVMValueRef of the LLVM Constant.
			return const_val;
		}
		case ast_var: {
			// Generate a load instruction that loads from the memory location (alloc instruction in var_map) corresponding to the variable name in the node.
			LLVMValueRef load_val = LLVMBuildLoad2(builder, LLVMInt32Type(), allocs[node->var.name], "");
			// Return the LLVMValueRef of the load instruction.
			return load_val;
		}
		case ast_uexpr: {
			// Generate LLVMValueRef for the expr in the unary expression node by calling genIRExpr recursively.
			LLVMValueRef expr = genIRExpr(node->uexpr.expr, builder);
			// Generate a subtraction instruction with constant 0 as the first operand and LLVMValueRef from step A above as the second operand.
			LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 1);
			LLVMValueRef subtract = LLVMBuildSub(builder, zero, expr, "");
			// Return the LLVMValueRef of the subtraction instruction.
			return subtract;
		}
		case ast_bexpr: {
			// Generate LLVMValueRef for the lhs in the binary expression node by calling genIRExpr recursively.
			LLVMValueRef lhs = genIRExpr(node->bexpr.lhs, builder);
			// Generate LLVMValueRef for the rhs in the binary expression node by calling genIRExpr recursively.
			LLVMValueRef rhs = genIRExpr(node->bexpr.rhs, builder);
			// Based on the operator in the binary expression node, generate an addition/subtraction/multiplication instruction using LLVMValueRef of lhs and rhs as operands.
			LLVMValueRef val;
			switch(node->bexpr.op) {
				case add: {
					val = LLVMBuildAdd(builder, lhs, rhs, "");
					break;
				}
				case sub: {
					val = LLVMBuildSub(builder, lhs, rhs, "");
					break;
				}
				case mul: {
					val = LLVMBuildMul(builder, lhs, rhs, "");
					break;
				}
				case divide: {
            		printf("genIRExpr bexpr has incorrect op type divide\n");
					val = LLVMBuildMul(builder, lhs, rhs, "");
					break;
				}
				case uminus: {
            		printf("genIRExpr bexpr has incorrect op type uminus\n");
					val = LLVMBuildMul(builder, lhs, rhs, "");
					break;
				}
			}
			// Return the LLVMValueRef of the instruction generated in step C.
            return val;
		}
		case ast_rexpr: {
			// Generate LLVMValueRef for the lhs in the binary expression node by calling genIRExpr recursively.
			LLVMValueRef lhs = genIRExpr(node->rexpr.lhs, builder);
			// Generate LLVMValueRef for the rhs in the binary expression node by calling genIRExpr recursively.
			LLVMValueRef rhs = genIRExpr(node->rexpr.rhs, builder);
			// Based on the operator in the relational expression node,
			// generate a compare instruction with parameter LLVMIntPredicate Op set to a comparison operator from LLVMIntPredicate enum in Core.h file.
			// The comparison operator should correspond to op in the given ast node.
			LLVMValueRef val;
			switch(node->rexpr.op) {
				case lt: {
					val = LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "");
					break;
				}
				case gt: {
					val = LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "");
					break;
				}
				case le: {
					val = LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "");
					break;
				}
				case ge: {
					val = LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "");
					break;
				}
				case eq: {
					val = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "");
					break;
				}
				case neq: {
					val = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "");
					break;
				}
			}
			// Return the LLVMValueRef of the instruction generated in step C.
			return val;
		}
		case ast_call: {
			// Generate a call instruction to read function.
			LLVMValueRef *read_args = NULL;
			LLVMValueRef read_call = LLVMBuildCall2(builder, ret_read, func_read, read_args, 0, "");
			// Return the LLVMValueRef of the call instruction
			return read_call;
		}
		default: {
            printf("genIRExpr node has incorrect type\n");
			// Generate a call instruction to read function.
			LLVMValueRef *read_args = NULL;
			LLVMValueRef read_call = LLVMBuildCall2(builder, ret_read, func_read, read_args, 0, "");
			// Return the LLVMValueRef of the call instruction
			return read_call;
		}
	}
}

LLVMBasicBlockRef genIRStmt(astNode *node, LLVMBuilderRef builder, LLVMBasicBlockRef start_bb) {
	astStmt stmt = node->stmt;
	
	switch(stmt.type) {
		case ast_asgn:	{
			LLVMPositionBuilderAtEnd(builder, start_bb);
			char *var_name = stmt.asgn.lhs->var.name;
			LLVMValueRef lhs = allocs[var_name];
			LLVMValueRef rhs = genIRExpr(stmt.asgn.rhs, builder);
			LLVMBuildStore(builder, rhs, lhs);
            return start_bb;
        }
		case ast_call: {
			LLVMPositionBuilderAtEnd(builder, start_bb);
			LLVMValueRef printed = genIRExpr(stmt.call.param, builder);
			LLVMBuildCall2(builder, ret_print, func_print, &printed, 1, "");
			return start_bb;
        }
		case ast_while: {
			// Set the position of the builder to the end of startBB.
			LLVMPositionBuilderAtEnd(builder, start_bb);
			// Generate a basic block to check the condition of the while loop. Let this be condBB.
			LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlock(func, "");
			// Generate an unconditional branch at the end of startBB to condBB.
			LLVMBuildBr(builder, cond_bb);
			// Set the position of the builder to the end of condBB.
			LLVMPositionBuilderAtEnd(builder, cond_bb);
			// Generate LLVMValueRef of the relational expression (comparison) in the condition of the while loop by calling the genIRExpr subroutine given below.
			LLVMValueRef cond_expr = genIRExpr(stmt.whilen.cond, builder);
			// Generate two basic blocks, trueBB and falseBB, that will be the successor basic blocks when condition is true or false respectively.
			LLVMBasicBlockRef true_bb = LLVMAppendBasicBlock(func, "");
			LLVMBasicBlockRef false_bb = LLVMAppendBasicBlock(func, "");
			// Generate a conditional branch at the end of condBB using the LLVMValueRef of comparison in step E above and setting the successor as trueBB when the condition is true and the successor as falseBB when the condition is false.
			LLVMBuildCondBr(builder, cond_expr, true_bb, false_bb);
			// Generate the LLVM IR for the while loop body by calling the genIRStmt subroutine recursively: pass trueBB as a parameter to genIRStmt and let the trueExitBB be the return value of genIRStmt call.
			LLVMBasicBlockRef true_exit_bb = genIRStmt(stmt.whilen.body, builder, true_bb);
			// Set the position of the builder to the end of trueExitBB.
			LLVMPositionBuilderAtEnd(builder, true_exit_bb);
			// Generate an unconditional branch to condBB at the end of trueExitBB.
			LLVMBuildBr(builder, cond_bb);
			// Return falseBB as the endBB for a while statement.
			return false_bb;
		}
		case ast_if: {
			// Set the position of the builder to the end of startBB.
			LLVMPositionBuilderAtEnd(builder, start_bb);
			// Generate LLVMValueRef of the relational expression (comparison) in the condition of the if statement by calling the genIRExpr subroutine given below.
			LLVMValueRef cond_expr = genIRExpr(stmt.ifn.cond, builder);
			// Generate two basic blocks, trueBB and falseBB, that will be the successor basic blocks when condition is true or false respectively.
			LLVMBasicBlockRef true_bb = LLVMAppendBasicBlock(func, "");
			LLVMBasicBlockRef false_bb = LLVMAppendBasicBlock(func, "");
			// Generate a conditional branch at the end of startBB using the LLVMValueRef of comparison in step B above, and setting the successor as trueBB when the condition is true and the successor as falseBB when the condition is false.
			LLVMBuildCondBr(builder, cond_expr, true_bb, false_bb);
			
			if (stmt.ifn.else_body == NULL) { // If there is no else part to the if statement
				// Generate the LLVM IR for the if body by calling the genIRStmt subroutine recursively: pass trueBB as a parameter to genIRStmt and let the ifExitBB be the return value of genIRStmt call.
				LLVMBasicBlockRef if_exit_bb = genIRStmt(stmt.ifn.if_body, builder, true_bb);
				// Set the position of the builder to the end of ifExitBB.
				LLVMPositionBuilderAtEnd(builder, if_exit_bb);
				// Add an unconditional branch to falseBB.
				LLVMBuildBr(builder, false_bb);
				// Return falseBB as the endBB.
				return false_bb;

			} else { // If there is an else part to the if statement
				// Generate the LLVM IR for the if body by calling the genIRStmt subroutine recursively: pass trueBB as a parameter to genIRStmt and let the ifExitBB be the return value of genIRStmt call.
				LLVMBasicBlockRef if_exit_bb = genIRStmt(stmt.ifn.if_body, builder, true_bb);
				// Generate the LLVM IR for the else body by calling the genIRStmt subroutine recursively: pass falseBB as a parameter to genIRStmt and let the elseExitBB be the return value of genIRStmt call.
				LLVMBasicBlockRef else_exit_bb = genIRStmt(stmt.ifn.else_body, builder, false_bb);
				// Generate a new endBB basic block.
				LLVMBasicBlockRef end_bb = LLVMAppendBasicBlock(func, "");
				// Set the position of the builder to the end of ifExitBB.
				LLVMPositionBuilderAtEnd(builder, if_exit_bb);
				// Add an unconditional branch to endBB.
				LLVMBuildBr(builder, end_bb);
				// Set the position of the builder to the end of elseExitBB.
				LLVMPositionBuilderAtEnd(builder, else_exit_bb);
				// Add an unconditional branch to endBB.
				LLVMBuildBr(builder, end_bb);
				// return endBB
				return end_bb;
			}
		}
		case ast_ret: {
			// Set the position of the builder to the end of startBB.
			LLVMPositionBuilderAtEnd(builder, start_bb);
			// Generate LLVMValueRef of the return value (could be an expression) by calling the genIRExpr subroutine given below.
			LLVMValueRef return_expr = genIRExpr(stmt.ret.expr, builder);
			// Generate a store instruction from LLVMValueRef of return value to the memory location pointed by ret_ref alloc statement.
			LLVMBuildStore(builder, return_expr, ret_ref);
			// Add an unconditional branch to retBB.
			LLVMBuildBr(builder, ret_bb);
			// Generate a new endBB basic block and return it
			LLVMBasicBlockRef end_bb = LLVMAppendBasicBlock(func, "");
			return end_bb;
		}
		case ast_block: {
			// Set a variable prevBB to startBB
			LLVMBasicBlockRef prev_bb = start_bb;
			// for each statement S in the statement list in the block statement:
			for (int i = 0; i < stmt.block.stmt_list->size(); i++) {
				astNode *block_node = (*(stmt.block.stmt_list))[i];
				// Generate the LLVM IR for S by calling the genIRStmt subroutine recursively:
				// pass prevBB as a parameter to genIRStmt and assign the return value of genIRStmt call to prevBB (this connects each statement to the next statement)
				prev_bb = genIRStmt(block_node, builder, prev_bb);
			}
			// return the value returned by the call to genIRStmt on the last statement in the statement list as endBB
			return prev_bb;
        }
		case ast_decl: {
			// ignore this
			return start_bb;
		}
		default: {
            printf("genIRStmt node has incorrect type\n");
			return start_bb;
		}
	}
}

int build_ir(astNode *rootAst, char *c_file) {
	// Creating a module 
	LLVMModuleRef mod = LLVMModuleCreateWithName(c_file);;
	LLVMSetTarget(mod, "x86_64-pc-linux-gnu");
    // Creating extern function declarations
    LLVMTypeRef param_read[] = {};
    ret_read = LLVMFunctionType(LLVMInt32Type(), param_read, 0, 0);
    func_read = LLVMAddFunction(mod, "read", ret_read);
    LLVMTypeRef param_print[] = { LLVMInt32Type() };
    ret_print = LLVMFunctionType(LLVMVoidType(), param_print, 0, 0);
    func_print = LLVMAddFunction(mod, "print", ret_print);

	// Creating a function with a module
	//// get function name
    astFunc ast_func = rootAst->prog.func->func;
	char* func_name = ast_func.name;
	//// get function param name if it exists
	astNode *func_param = ast_func.param;
	LLVMTypeRef ret_type;
	if (func_param == NULL) {
		LLVMTypeRef param_types[] = { };
		ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
	} else {
		LLVMTypeRef param_types[] = { LLVMInt32Type() };
		ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 1, 0);
	}
	func = LLVMAddFunction(mod, func_name, ret_type);

	//Creating a basic block
	LLVMBasicBlockRef entry_bb = LLVMAppendBasicBlock(func, "");

	//All instructions need to be created using a builder. The builder specifies
	//where the instructions are added.
	LLVMBuilderRef builder = LLVMCreateBuilder();
	LLVMPositionBuilderAtEnd(builder, entry_bb);
	
	// Part 1: preprocessor
	unordered_map<string, string> replace;
	
	// record the parameter if it exists
	if (func_param != NULL) {
		string param_name = ast_func.param->var.name;
		
		// Creating an alloc instruction with alignment
		LLVMValueRef alloc = LLVMBuildAlloca(builder, LLVMInt32Type(), ast_func.param->var.name); 
		LLVMSetAlignment(alloc, 4);
		allocs[param_name] = alloc;
		//printf("%s\n", param_name);
	}
		
	// rename non-unique variable names and allocate all variables
	replace_scope_names(ast_func.body, &allocs, &replace, &builder);

	// make an alloc for the return value
	ret_ref = LLVMBuildAlloca(builder, LLVMInt32Type(), "return"); 
	LLVMSetAlignment(ret_ref, 4);

	// generate store instruction to store the function parameter
	if (func_param != NULL) {
		string param_name = ast_func.param->var.name;
		LLVMBuildStore(builder, LLVMGetParam(func, 0), allocs[param_name]);
	}

	// make a return bb
	ret_bb = LLVMAppendBasicBlock(func, "");
	LLVMPositionBuilderAtEnd(builder, ret_bb);
	LLVMValueRef return_load = LLVMBuildLoad2(builder, LLVMInt32Type(), ret_ref, "");	
	LLVMBuildRet(builder, return_load);
	
	// call genIRStmt
	LLVMBasicBlockRef exit_bb = genIRStmt(ast_func.body, builder, entry_bb);

	LLVMValueRef exit_bb_term = LLVMGetBasicBlockTerminator(exit_bb);
	if (exit_bb_term == NULL) {
		LLVMPositionBuilderAtEnd(builder, exit_bb);
		LLVMBuildBr(builder, ret_bb);
	}

	// Remove all basic blocks that do not have any predecessor basic blocks
	bool did_remove_bb = false;
	unordered_map<LLVMBasicBlockRef, set<LLVMBasicBlockRef>> predecessors;
	do {
		did_remove_bb = false;
		//// calculate predecessors
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(func); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
			LLVMValueRef terminator = LLVMGetBasicBlockTerminator(basic_block);
			int num_successors = LLVMGetNumSuccessors(terminator);
			for (int i = 0; i < num_successors; i++) {
				LLVMBasicBlockRef successor = LLVMGetSuccessor(terminator, i);
				predecessors[successor].insert(basic_block);
			}
		}
		/// remove bbs that do not have predecessors except the first basic block
		LLVMBasicBlockRef second_bb = LLVMGetNextBasicBlock(LLVMGetFirstBasicBlock(func));
		if (second_bb != NULL) {
			for (LLVMBasicBlockRef basic_block = second_bb; basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
				if (predecessors[basic_block].size() == 0) {
					LLVMDeleteBasicBlock(basic_block);
					did_remove_bb = true;
					break;
				}
			}
		}
		predecessors.clear();
	} while (did_remove_bb);

	//// memory cleanup
  	LLVMDisposeBuilder(builder);
 	allocs.clear();
	LLVMPrintModuleToFile(mod, "output_unoptimized.ll", NULL);
	LLVMDisposeModule(mod);
	LLVMShutdown();
	return 0;
}
