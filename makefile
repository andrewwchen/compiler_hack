MINIC = minic_compiler
TEST = opt_tests/test
LLVMCODE = llvm_parser

minic_compiler.out:
	yacc -d -Wcounterexamples frontend.y 
	lex frontend.l
	g++ -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c
	g++ -g lex.yy.c y.tab.c ast.c minic_compiler.c semantic_analysis.c -o minic_compiler.out $(LLVMCODE).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@


llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf test_new.ll
	rm -rf *.o
	rm -rf *.out
	rm -rf lex.yy.c y.tab.c y.tab.h 
