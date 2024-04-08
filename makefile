filename = minic_compiler
(filename).out: $(filename).l
	yacc -d -Wcounterexamples $(filename).y 
	lex $(filename).l
	g++ -g lex.yy.c y.tab.c -o $(filename).out

clean:
	rm -rf $(filename).out lex.yy.c y.tab.c y.tab.h
