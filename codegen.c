#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cstddef>
#include <vector>
#include <set>
#include <tuple>
#include <unordered_map> 
#include <bits/stdc++.h> 
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
using namespace std;


unordered_map<LLVMValueRef, int> inst_index;
unordered_map<LLVMValueRef, int> inst_index_all;
unordered_map<LLVMValueRef, tuple<int, int>> live_range;
vector<LLVMValueRef> sorted_list;
unordered_map<LLVMValueRef, int> reg_map;


LLVMModuleRef makeLLVMModel(char * filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	//LLVMDisposeMemoryBuffer(ll_f);

	if (err != NULL) { 
		printf("%s\n", err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		printf("%s\n", err);
		//LLVMDisposeMessage(err);
	}

	return m;
}

bool instruction_has_lhs(LLVMValueRef instruction) {
	LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
	return (op == LLVMLoad|| op == LLVMICmp
		|| op == LLVMAdd || op == LLVMMul || op == LLVMSub || op == LLVMSDiv
		|| (op == LLVMCall && LLVMGetNumOperands(instruction) == 1)); // read call
}

int value_range_end(LLVMValueRef value) {
	if (live_range.count(value) == 0) {
		printf("value_range_end: error\n");
		return -1;
	}
	int range_end = get<1>(live_range[value]);
	return range_end;
}

bool operand_live_range_ends(LLVMValueRef operand, LLVMValueRef instruction) {
	if (inst_index_all.count(instruction) == 0) {
		printf("operand_live_range_ends: instruction doesn't have an index\n");
		return false;
	}
	if (live_range.count(operand) == 0) {
		return false;
	}
	int index = inst_index_all[instruction];
	int range_end = value_range_end(operand);
	return index == range_end;
}

bool operand_has_physical_register(LLVMValueRef operand) {
	if (reg_map.count(operand) == 0) {
		return false;
	}
	int reg = reg_map[operand];
	return reg != -1;
}

bool greater_range_end(LLVMValueRef v1, LLVMValueRef v2) 
{ 
	int v1_end = value_range_end(v1);
	int v2_end = value_range_end(v2);
	return (v1_end > v2_end); 
}

int compute_liveness(LLVMBasicBlockRef basic_block) {
	int index = 0;
	inst_index.clear();
	live_range.clear();
	sorted_list.clear();
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block); instruction; instruction = LLVMGetNextInstruction(instruction)) {

		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
		// ignore alloc
		if (op == LLVMAlloca) {
			continue;
		}
		inst_index_all[instruction] = index;
		if (instruction_has_lhs(instruction)) {
			inst_index[instruction] = index;
			live_range[instruction] = make_tuple(index, index);
		}
		
		int num_operands = LLVMGetNumOperands(instruction);
		for (int i = 0; i < num_operands; i++) {
			LLVMValueRef operand = LLVMGetOperand(instruction, i);
			if (live_range.count(operand)) {
				get<1>(live_range[operand]) = index;
			}
		}

		index++;
	}

	sorted_list.resize(inst_index.size());
	index = 0;
	for (unordered_map<LLVMValueRef,int>::iterator iter = inst_index.begin(); iter != inst_index.end(); ++iter) {
		LLVMValueRef instruction =  iter->first;
		sorted_list[index] = instruction;
		index++;
	}


	sort(sorted_list.begin(), sorted_list.end(), greater_range_end);

	for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block); instruction; instruction = LLVMGetNextInstruction(instruction)) {

		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
		// ignore alloc
		if (op == LLVMAlloca) {
			continue;
		}
		if (instruction_has_lhs(instruction)) {
			printf("%d\n", value_range_end(instruction));
		}
		
		index++;
	}

	return 0;
}

LLVMValueRef find_spill() {
	for(int i = 0; i < sorted_list.size(); i++){
        LLVMValueRef instruction = sorted_list[i];
		if(operand_has_physical_register(instruction)) {
			return instruction;
		}
    }
	printf("find_spill: could not find instruction\n");
	return NULL;
}

int register_allocation(LLVMModuleRef m) {

	for (LLVMValueRef function =  LLVMGetFirstFunction(m); function; function = LLVMGetNextFunction(function)) {
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
			set<int> available_regs({0,1,2});
			compute_liveness(basic_block);
			for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block); instruction; instruction = LLVMGetNextInstruction(instruction)) {
				LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
				// Ignore Instr if it is an alloc instruction
				if (op == LLVMAlloca) {
					continue;
				}
				// If Instr is an instruction that does not have a result
				if (!instruction_has_lhs(instruction)) {
					printf("no lhs\n");
					int num_operands = LLVMGetNumOperands(instruction);
					for (int i = 0; i < num_operands; i++) {
						LLVMValueRef operand = LLVMGetOperand(instruction, i);
						// If live range of any operand of Instr ends,
						if (operand_live_range_ends(operand, instruction)) {
							// and if it has a physical register P assigned to it
							if (operand_has_physical_register(operand)) {
								// then add P to available set of registers.
								int P = reg_map[operand];
								printf("no lhs adding reg %d back\n", P);
								available_regs.insert(P);
							}
						}
					}
				// If instruction Instr is not ignored above,
				// then one of the three following cases may occur:
				} else if ( // If following conditions about Instr  hold:
				 	// is of type add/sub/mul
					(op == LLVMAdd || op == LLVMMul || op == LLVMSub)
					// the first operand has a physical register R assigned to it in reg_map
					&& operand_has_physical_register(LLVMGetOperand(instruction, 0))
					// liveness range of the first operand ends at Instr
					&& operand_live_range_ends(LLVMGetOperand(instruction, 0), instruction)
				) {
					// Add the entry Instr -> R to reg_map (Note: Instr will be a LLVMValueRef)
					int R = reg_map[LLVMGetOperand(instruction, 0)];
					reg_map[instruction] = R;
					printf("add/sub/mul assigning reg %d\n", R);
					// If live range of second operand of Instr ends
					// and it has a physical register P assigned to it then add P to available set of registers.
					LLVMValueRef second_operand = LLVMGetOperand(instruction, 1);
					if (operand_live_range_ends(second_operand, instruction) && operand_has_physical_register(second_operand)) {
						int P = reg_map[second_operand];
						available_regs.insert(P);
						printf("add/sub/mul adding reg %d back\n", P);
					}

				// If a physical register R is available
				} else if (available_regs.size() > 0) {
					// Remove R from the available pool of registers
					int R = *available_regs.begin();
					available_regs.erase(R);
					printf("avail assigning reg %d\n", R);
					// Add the entry Instr -> R to reg_map (Note: Instr will be a LLVMValueRef)
					reg_map[instruction] = R;
					// If live range of any operand of Instr ends,
					// and it has a physical register P assigned to it
					// then add P to available set of registers.
					int num_operands = LLVMGetNumOperands(instruction);
					for (int i = 0; i < num_operands; i++) {
						LLVMValueRef operand = LLVMGetOperand(instruction, i);
						if (operand_live_range_ends(operand, instruction) && operand_has_physical_register(operand)) {
							int P = reg_map[operand];
							available_regs.insert(P);
							printf("avail adding reg %d back\n", P);
						}
					}
				// If a physical register is not available:
				} else {
					// Call find_spill to find the LLVMValueRef to spill based on heuristic, let this LLVMValueRef be V
					LLVMValueRef V = find_spill();
					// If V has more uses than Instr (or liveness range of Instr ends after liveness range of V,
					// note the choice is based on your heuristic to spill)
					int v_end = value_range_end(V);
					int instruction_end = value_range_end(V);
					if (instruction_end > v_end) {
						// Add the entry Instr -> -1 to reg_map to indicate that it does not have a physical register assigned
						reg_map[instruction] = -1;
						printf("spill\n");
					} else {
						// Get the physical register R assigned to V in reg_map
						int R = reg_map[V];
						// Add the entry Instr -> R to reg_map
						reg_map[instruction] = R;
						// Update the entry for V from V -> R to  V -> -1 in reg_map
						reg_map[V] = -1;
						printf("spill\n");
					}
					// If the live range of any operand of Instr ends,
					// and it has a physical register P assigned to it,
					// then add P to the available set of registers.
					int num_operands = LLVMGetNumOperands(instruction);
					for (int i = 0; i < num_operands; i++) {
						LLVMValueRef operand = LLVMGetOperand(instruction, i);
						if (operand_live_range_ends(operand, instruction) && operand_has_physical_register(operand)) {
							int P = reg_map[operand];
							available_regs.insert(P);
							printf("adding reg %d back\n", P);
						}
					}
				}
			}
		}
 	}

	
	for (LLVMValueRef function =  LLVMGetFirstFunction(m); function; function = LLVMGetNextFunction(function)) {
		int bb = 0;
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
			int index = 0;
			for (LLVMValueRef instruction = LLVMGetFirstInstruction(basic_block); instruction; instruction = LLVMGetNextInstruction(instruction)) {
				
				LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
				if (op == LLVMAlloca) {
					continue;
				}
				if (instruction_has_lhs(instruction)) {

					printf("bb%d-%d: %d\n", bb, index, reg_map[instruction]);
				}
				index++;
			}
			bb++;
		}
	}
	return 0;
}

int get_offset_map(LLVMModuleRef m, unordered_map<LLVMValueRef, int> *offset_map) {
	
	for (LLVMValueRef function =  LLVMGetFirstFunction(m); function; function = LLVMGetNextFunction(function)) {
		// Initialize localMem to 4.
		int local_mem = 4;
		// If the function has a parameter
		// Get the LLVMValueRef of the parameter using LLVMGetParam.
		// Add the LLVMValueRef of the parameter as key and its associated value as 8 to the offset_map.
		if (LLVMCountParams(function) > 0) {
			LLVMValueRef param = LLVMGetParam(function, 0);
			(*offset_map)[param] = 8;
		}
		
		for (LLVMBasicBlockRef basic_block = LLVMGetFirstBasicBlock(function); basic_block; basic_block = LLVMGetNextBasicBlock(basic_block)) {
			for (LLVMValueRef instruction = LLVMGetLastInstruction(basic_block); instruction; instruction = LLVMGetPreviousInstruction(instruction)) {
				LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
				if (op == LLVMAlloca) {
					// Add 4 to the localMem.
					local_mem += 4;
					// Add instr as the key with the associated value as (-1*localMem) to offset_map.
					(*offset_map)[instruction] = -1*local_mem;
					
				} else if (op = LLVMStore) {
					LLVMValueRef first_operand = LLVMGetOperand(instruction, 0);
					LLVMValueRef second_operand = LLVMGetOperand(instruction, 1);
					// If the first operand of the store instruction is equal to the function parameter
					if (LLVMCountParams(function) > 0 && first_operand == LLVMGetParam(function, 0)) {
						// Get the value associated with the first operand in offset_map. Let this be x.
						// Change the value associated with the second operand in offset_map to x.
						int x = (*offset_map)[first_operand];
						(*offset_map)[second_operand] = x;
					// If the first operand of the store instruction is not equal to the function parameter
					} else {
						// Get the value associated with the second operand in offset_map. Let this be x.
						int x = (*offset_map)[second_operand];
						// Add the first operand as the key with the associated value as x in offset_map.
						(*offset_map)[first_operand] = x;
					}
				} else if (op = LLVMLoad) {
					LLVMValueRef first_operand = LLVMGetOperand(instruction, 0);
					// Get the value associated with the first operand in offset_map. Let this be x.
					int x = (*offset_map)[first_operand];
					// Add instr as the key with the associated value as x in the offset_map.
					(*offset_map)[instruction] = x;
				}
			}
		}
	}

}

int gen_assembly_code(char *optimized_ll_file) {
	LLVMModuleRef m;
	//char ll_file[] = "output_optimized.ll";
	m = makeLLVMModel(optimized_ll_file);

	if (m == NULL){
		printf("err\n");
		
	    fprintf(stderr, "m is NULL\n");
		return -1;
	}

	if (register_allocation(m) == -1) return -1;

	unordered_map<LLVMValueRef, int> offset_map;
	if (get_offset_map(m, &offset_map) == -1) return -1;

	// LLVMPrintModuleToFile (m, "final.ll", NULL);
	LLVMDisposeModule(m);
	LLVMShutdown();

	return 0;
}