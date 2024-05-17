#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cstddef>
#include <vector>
#include <set>
#include <unordered_map> 
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
using namespace std; 

#define prt(x) if(x) { printf("%s\n", x); }

/* This function reads the given llvm file and loads the LLVM IR into
	 data-structures that we can works on for optimization phase.
*/
 
LLVMModuleRef createLLVMModel(char * filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	//LLVMDisposeMemoryBuffer(ll_f);

	if (err != NULL) { 
		prt(err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		prt(err);
		//LLVMDisposeMessage(err);
	}

	return m;
}
/*
bool are_operands_equal(LLVMValueRef operand1, LLVMValueRef operand2) {
	LLVMOpcode op1 = LLVMGetInstructionOpcode(operand1);
	LLVMOpcode op2 = LLVMGetInstructionOpcode(operand2);
	if (op1 != op2) return false;
	if num_operands = LLVMGetNumOperands(instruction);
}*/

bool elimCommonSubexpr(LLVMBasicBlockRef bb) {
	bool did_optimize = false;
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

		// do not perform subexpression elimination on ALLOC
		if (op == LLVMAlloca) {
			continue;
		}

		int num_operands = LLVMGetNumOperands(instruction);
		LLVMValueRef operands[num_operands];
		for (int i = 0; i < num_operands; i++) {
			LLVMValueRef operand = LLVMGetOperand(instruction, i);
			operands[i] = operand;
		}

		LLVMValueRef next_instruction = LLVMGetNextInstruction(instruction);
		while ( next_instruction ) {
			LLVMOpcode next_op = LLVMGetInstructionOpcode(next_instruction);
			int next_num_operands = LLVMGetNumOperands(next_instruction);
			LLVMValueRef next_operands[num_operands];
			for (int i = 0; i < num_operands; i++) {
				LLVMValueRef next_operand = LLVMGetOperand(next_instruction, i);
				next_operands[i] = next_operand;
			}

			// stop eliminating common expr if the subexpression is a LOAD and an operand is changed by a STORE
			if (next_op == LLVMStore && op == LLVMLoad) {
				bool is_operand_changed = false;
				for (int i = 0; i < num_operands; i++) {
					for (int j = 0; j < num_operands; j++) {
						if (operands[i] != next_operands[j]) {
							is_operand_changed = true;
						}
					}
				}
				if (is_operand_changed) {
					break;
				}
			}

			if (next_op == op && next_num_operands == num_operands) {

				// order of operands does not matter for these two operations
				if (op == LLVMAdd || op == LLVMMul) { 
					if ((next_operands[0] == operands[0] && next_operands[1] == operands[1]) || (next_operands[0] == operands[1] && next_operands[1] == operands[0])) {
						LLVMReplaceAllUsesWith(next_instruction, instruction);
						did_optimize = true;
					}
				// order of operands does matter for the remaining operations
				} else {
					bool are_operands_equal = true;
					for (int i = 0; i < num_operands; i++) {
						if (next_operands[i] != operands[i]) {
							are_operands_equal = false;
							break;
						}
					}
					if (are_operands_equal) {
						LLVMReplaceAllUsesWith(next_instruction, instruction);
						did_optimize = true;
					}
				}
			}
			next_instruction = LLVMGetNextInstruction(next_instruction);
		}
	}
	return did_optimize;
}

bool elimDeadCode(LLVMBasicBlockRef bb) {
	bool did_optimize = false;

	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

		// do not perform subexpression elimination on STORE, CALL, BRANCH, or RET
		if (op == LLVMStore || op == LLVMCall || op == LLVMRet || op == LLVMBr || op == LLVMAlloca) {
			continue;
		}

		// check if this instruction is ever used
		bool is_instruction_used = false;

		LLVMValueRef next_instruction = LLVMGetNextInstruction(instruction);
		while ( next_instruction ) { 
			int next_num_operands = LLVMGetNumOperands(next_instruction);
			for (int i = 0; i < next_num_operands; i++) {
				LLVMValueRef next_operand = LLVMGetOperand(next_instruction, i);
				if (next_operand == instruction) {
					is_instruction_used = true;
					break;
				}
			}
			next_instruction = LLVMGetNextInstruction(next_instruction);
		}

		// if the instruction is not used, delete the instruction
		if (!is_instruction_used) {
			LLVMInstructionEraseFromParent(instruction);
			did_optimize = true;
		}
	}
	return did_optimize;
}

bool foldConstants(LLVMBasicBlockRef bb) {
	bool did_optimize = false;

	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

		if (op == LLVMAdd && LLVMIsConstant(LLVMGetOperand(instruction, 0)) && LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
			LLVMValueRef newConst = LLVMConstAdd(LLVMGetOperand(instruction,0), LLVMGetOperand(instruction,1));
			LLVMReplaceAllUsesWith(instruction, newConst);
			did_optimize = true;
      	} else if (op == LLVMMul && LLVMIsConstant(LLVMGetOperand(instruction, 0)) && LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
			LLVMValueRef newConst = LLVMConstMul(LLVMGetOperand(instruction,0), LLVMGetOperand(instruction,1));
			LLVMReplaceAllUsesWith(instruction, newConst);
			did_optimize = true;
      	} else if (op == LLVMSub && LLVMIsConstant(LLVMGetOperand(instruction, 0)) && LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
			LLVMValueRef newConst = LLVMConstSub(LLVMGetOperand(instruction,0), LLVMGetOperand(instruction,1));
			LLVMReplaceAllUsesWith(instruction, newConst);
			did_optimize = true;
      	}
	}
	return did_optimize;
}

void optimize(LLVMValueRef function) {
	// calculate predecessors
	unordered_map<LLVMBasicBlockRef, set<LLVMBasicBlockRef>> predecessors;
	for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
		LLVMValueRef terminator = LLVMGetBasicBlockTerminator(basic_block);
		int num_successors = LLVMGetNumSuccessors(terminator);
		for (int i = 0; i < num_successors; i++) {
			LLVMBasicBlockRef successor = LLVMGetSuccessor(terminator, i);
			predecessors[successor].insert(basic_block);
		}
	}
	bool did_optimize_func;
	do {
		did_optimize_func = false;

		// globally optimize
		unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> gen;
		unordered_map<LLVMValueRef, set<LLVMValueRef>> var_to_store_instructions;
		unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> kill;
		unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> in;
		unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> out;
		unordered_map<LLVMBasicBlockRef, set<LLVMValueRef>> old_out;
		set<LLVMValueRef> marked_for_deletion;

		// calculate gen set and var_to_store_instructions set out to gen
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
			for (LLVMValueRef instruction = LLVMGetLastInstruction(basic_block); instruction; instruction = LLVMGetPreviousInstruction(instruction)) {
				LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
				if (op == LLVMStore) {
					LLVMValueRef var = LLVMGetOperand(instruction, 1);
					var_to_store_instructions[var].insert(instruction);
					bool is_var_in_gen = false;
					for (LLVMValueRef gen_instruction : gen[basic_block]) {
						LLVMValueRef gen_var = LLVMGetOperand(gen_instruction, 1);
						if (gen_var == var) {
							bool is_var_in_gen = true;
							break;
						}
					}
					if (!is_var_in_gen) {
						gen[basic_block].insert(instruction);
					}
				}
			}
			out[basic_block].insert(gen[basic_block].begin(), gen[basic_block].end());
		}

		// calculate kill set
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
			for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block); instruction; instruction = LLVMGetNextInstruction(instruction)) {
				LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
				if (op == LLVMStore) {
					LLVMValueRef var = LLVMGetOperand(instruction, 1);
					for (LLVMValueRef store_instruction : var_to_store_instructions[var]) {
						if (store_instruction != instruction) {
							kill[basic_block].insert(store_instruction);
						}
					}
				}
			}
		}

		int did_out_change;
		do {
			// calculate in
			for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
				in[basic_block].clear();
				for (LLVMBasicBlockRef predecessor : predecessors[basic_block]) 
				{
					in[basic_block].insert(out[predecessor].begin(), out[predecessor].end());
				}  
			}
			
			// calculate out
			did_out_change = false;
			for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
				old_out[basic_block].clear();
				old_out[basic_block].insert(out[basic_block].begin(), out[basic_block].end());
				out[basic_block].clear();
				out[basic_block].insert(in[basic_block].begin(), in[basic_block].end());
				for (LLVMValueRef killed : kill[basic_block]) {
					out[basic_block].erase(killed);
				}
				out[basic_block].insert(gen[basic_block].begin(), gen[basic_block].end());
				if (old_out != out) {
					did_out_change = true;
				}
			}
			
		} while (did_out_change);
		
		// use in and out to optimize
		
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
			
			for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block); instruction; instruction = LLVMGetNextInstruction(instruction)) {
				LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
				if (op == LLVMStore) {
					LLVMValueRef var = LLVMGetOperand(instruction, 1);
					// remove all stores killed by instruction from R
					set<LLVMValueRef> store_instructions = var_to_store_instructions[var];
					for (LLVMValueRef store_instruction : store_instructions) {
						if (store_instruction != instruction) {
							in[basic_block].erase(store_instruction);
						}
					}
					// add instruction to R
					in[basic_block].insert(instruction);
				}
				else if (op == LLVMLoad) {
					LLVMValueRef var = LLVMGetOperand(instruction, 0);
					LLVMValueRef prev_val = NULL;
					bool should_replace_prev_val = false;
					// check all stores in R to the same address we are loading from to see if they are all the same constant
					for (LLVMValueRef store_instruction : in[basic_block]) {
						LLVMValueRef store_var = LLVMGetOperand(store_instruction, 1);
						if (var == store_var) {
							LLVMValueRef val = LLVMGetOperand(store_instruction, 0);
							if (LLVMIsConstant(val)) {
								if (should_replace_prev_val) {
									if (val != prev_val) {
										should_replace_prev_val = false;
									}
								} else if (prev_val == NULL) {
									should_replace_prev_val = true;
									prev_val = val;
								}
							}
						}
					}
					if (should_replace_prev_val) {
						marked_for_deletion.insert(instruction);
						LLVMReplaceAllUsesWith(instruction, prev_val);
						did_optimize_func = true;
					}
				}
				for (LLVMValueRef instruction : marked_for_deletion) {
					LLVMInstructionEraseFromParent(instruction);
				}
				marked_for_deletion.clear();
			}

		}
		
		
		// locally optimize
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function);
				basic_block;
				basic_block = LLVMGetNextBasicBlock(basic_block)) {
			
			bool did_optimize_bb;
			do {
				bool did_elim_common_subexpr = elimCommonSubexpr(basic_block);
				bool did_elim_dead_code = elimDeadCode(basic_block);
				bool did_fold_constants = foldConstants(basic_block);
				did_optimize_bb = did_elim_common_subexpr || did_elim_dead_code || did_fold_constants;
				did_optimize_func = did_optimize_func || did_optimize_bb;
			} while (did_optimize_bb);
		}
	} while (did_optimize_func);
}

void walkFunctions(LLVMModuleRef module){
	for (LLVMValueRef function =  LLVMGetFirstFunction(module); 
			function; 
			function = LLVMGetNextFunction(function)) {

		optimize(function);
 	}
}

int optimize_llvm(char* ll_file)
{
	LLVMModuleRef m;

	m = createLLVMModel(ll_file);

	if (m != NULL){
		walkFunctions(m);
		LLVMPrintModuleToFile (m, "output_optimized.ll", NULL);
		LLVMDisposeModule(m);
		LLVMShutdown();
		return 0;
	}
	else {
	    fprintf(stderr, "m is NULL\n");
		return -1;
	}
}
