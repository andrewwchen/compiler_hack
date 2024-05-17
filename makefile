TEST = opt_tests/test

minic_compiler.out: llvm_parser.c ir.c semantic_analysis.c minic_compiler.c
	yacc -d -Wcounterexamples frontend.y 
	lex frontend.l
	g++ -g -I /usr/include/llvm-c-15/ -c llvm_parser.c
	g++ -g lex.yy.c y.tab.c ast.c minic_compiler.c semantic_analysis.c ir.c -o minic_compiler.out llvm_parser.o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@


llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf output_unoptimized.ll
	rm -rf output_optimized.ll
	rm -rf *.o
	rm -rf *.out
	rm -rf lex.yy.c y.tab.c y.tab.h 
