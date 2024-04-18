#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

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
bool areOperandsEqual(LLVMValueRef operand1, LLVMValueRef operand2) {
	LLVMOpcode op1 = LLVMGetInstructionOpcode(operand1);
	LLVMOpcode op2 = LLVMGetInstructionOpcode(operand2);
	if (op1 != op2) return false;
	if num_operands = LLVMGetNumOperands(instruction);
}*/

bool elimCommonSubexpr(LLVMBasicBlockRef bb) {
	bool didOptimize = false;
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
				bool isOperandChanged = false;
				for (int i = 0; i < num_operands; i++) {
					for (int j = 0; j < num_operands; j++) {
						if (operands[i] == next_operands[j]) {
							isOperandChanged = true;
						}
					}
				}
				if (isOperandChanged) {
					break;
				}
			}

			if (next_op == op && next_num_operands == num_operands) {

				// order of operands does not matter for these two operations
				if (op == LLVMAdd || op == LLVMMul) { 
					if ((next_operands[0] == operands[0] && next_operands[1] == operands[1]) || (next_operands[0] == operands[1] && next_operands[1] == operands[0])) {
						LLVMReplaceAllUsesWith(next_instruction, instruction);
						didOptimize = true;
					}
				// order of operands does matter for the remaining operations
				} else {
					bool areOperandsEqual = true;
					for (int i = 0; i < num_operands; i++) {
						if (next_operands[i] != operands[i]) {
							areOperandsEqual = false;
							break;
						}
					}
					if (areOperandsEqual) {
						LLVMReplaceAllUsesWith(next_instruction, instruction);
						didOptimize = true;
					}
				}
			}
			next_instruction = LLVMGetNextInstruction(next_instruction);
		}
	}
	return didOptimize;
}

bool elimDeadCode(LLVMBasicBlockRef bb) {
	bool didOptimize = false;

	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

		// do not perform subexpression elimination on STORE, CALL, BRANCH, or RET
		if (op == LLVMStore || op == LLVMInvoke || op == LLVMRet || op == LLVMBr) {
			continue;
		}

		// check if this instruction is ever used
		bool isInstructionUsed = false;

		LLVMValueRef next_instruction = LLVMGetNextInstruction(instruction);
		while ( next_instruction ) { 
			int next_num_operands = LLVMGetNumOperands(next_instruction);
			for (int i = 0; i < next_num_operands; i++) {
				LLVMValueRef next_operand = LLVMGetOperand(next_instruction, i);
				if (next_operand == instruction) {
					isInstructionUsed = true;
					break;
				}
			}
			next_instruction = LLVMGetNextInstruction(next_instruction);
		}

		// if the instruction is not used, delete the instruction
		if (!isInstructionUsed) {
			LLVMInstructionEraseFromParent(instruction);
			didOptimize = true;
		}
	}
	return didOptimize;
}

bool foldConstants(LLVMBasicBlockRef bb) {
	bool didOptimize = false;

	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);

		if (op == LLVMAdd && LLVMIsConstant(LLVMGetOperand(instruction, 0)) && LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
			LLVMValueRef newConst = LLVMConstAdd(LLVMGetOperand(instruction,0), LLVMGetOperand(instruction,1));
			LLVMReplaceAllUsesWith(instruction, newConst);
			didOptimize = true;
      	} else if (op == LLVMMul && LLVMIsConstant(LLVMGetOperand(instruction, 0)) && LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
			LLVMValueRef newConst = LLVMConstMul(LLVMGetOperand(instruction,0), LLVMGetOperand(instruction,1));
			LLVMReplaceAllUsesWith(instruction, newConst);
			didOptimize = true;
      	} else if (op == LLVMSub && LLVMIsConstant(LLVMGetOperand(instruction, 0)) && LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
			LLVMValueRef newConst = LLVMConstSub(LLVMGetOperand(instruction,0), LLVMGetOperand(instruction,1));
			LLVMReplaceAllUsesWith(instruction, newConst);
			didOptimize = true;
      	}
	}
	return didOptimize;
}

void optimize(LLVMValueRef function){
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
		bool didOptimize;
		do {
			bool didElimCommonSubexpr = elimCommonSubexpr(basicBlock);
			bool didElimDeadCode = elimDeadCode(basicBlock);
			bool didFoldConstants = foldConstants(basicBlock);
			didOptimize = didElimCommonSubexpr || didElimDeadCode;
		} while (didOptimize);
	
	}
}

void walkFunctions(LLVMModuleRef module){
	for (LLVMValueRef function =  LLVMGetFirstFunction(module); 
			function; 
			function = LLVMGetNextFunction(function)) {

		optimize(function);
 	}
}

void walkGlobalValues(LLVMModuleRef module){
	for (LLVMValueRef gVal =  LLVMGetFirstGlobal(module);
                        gVal;
                        gVal = LLVMGetNextGlobal(gVal)) {

                const char* gName = LLVMGetValueName(gVal);
                printf("Global variable name: %s\n", gName);
        }
}

int main(int argc, char** argv)
{
	LLVMModuleRef m;

	if (argc == 2){
		m = createLLVMModel(argv[1]);
	}
	else{
		m = NULL;
		return 1;
	}

	if (m != NULL){
		//LLVMDumpModule(m);
		walkGlobalValues(m);
		walkFunctions(m);
		LLVMPrintModuleToFile (m, "test_new.ll", NULL);
		//LLVMDisposeModule(m);
	}
	else {
	    fprintf(stderr, "m is NULL\n");
	}
	
	return 0;
}
