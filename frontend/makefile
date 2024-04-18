filename = frontend
(filename).out: $(filename).l
	yacc -d -Wcounterexamples $(filename).y 
	lex $(filename).l
	g++ -g lex.yy.c y.tab.c ast.c $(filename).c semantic_analysis.c -o $(filename).out

clean:
	rm -rf $(filename).out lex.yy.c y.tab.c y.tab.h 
