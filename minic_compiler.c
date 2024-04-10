#include<stdio.h>
#include"ast.h"
#include"semantic_analysis.h"

extern int yylex_destroy();
extern int yyparse (astNode **rootAstPtr);
extern FILE * yyin;

int main(int argc, char* argv[]){
		if (argc == 2){
			yyin = fopen(argv[1], "r");
			if (yyin == NULL) {
				fprintf(stderr, "File open error\n");
				return 1;
			}
		}
        astNode *rootAst;
		yyparse(&rootAst);
		printNode(rootAst, 1);
        printf("static analysis result: %d\n", perform_analysis(rootAst));
		freeNode(rootAst);
		if (argc == 2) fclose(yyin);
		yylex_destroy();
		return 0;
}