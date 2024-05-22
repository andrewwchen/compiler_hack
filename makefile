TEST = opt_tests/test

main.out: compare.c semantic_analysis.c main.c
	yacc -d -Wcounterexamples frontend.y 
	lex frontend.l
	g++ -g lex.yy.c y.tab.c ast.c main.c semantic_analysis.c compare.c -o main.out


llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf output_unoptimized.ll
	rm -rf output_optimized.ll
	rm -rf *.o
	rm -rf *.out
	rm -rf lex.yy.c y.tab.c y.tab.h
