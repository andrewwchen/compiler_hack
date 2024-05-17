#include<stdio.h>
#include"ast.h"
#include"ir.h"
#include"semantic_analysis.h"
#include"llvm_parser.h"

extern int yylex_destroy();
extern int yyparse (astNode **rootAstPtr);
extern FILE * yyin;

int main(int argc, char** argv)
{
	if (argc != 3){
        printf("Usage: ./minic_compiler.out {.c file} {.ll file}\n");
        return -1;
    }
    char* c_file = argv[1];
    char* ll_file = argv[2];

    yyin = fopen(c_file, "r");
    if (yyin == NULL) {
        fprintf(stderr, "File open error\n");
        yylex_destroy();
        return -1;
    }
    astNode *rootAst;
    yyparse(&rootAst);
    //printNode(rootAst, 1);
    printf("static analysis result of .c file: %d\n", perform_analysis(rootAst));

    printf("ir build result: %d\n", build_ir(rootAst, c_file));
    //printNode(rootAst, 1);

    freeNode(rootAst);
    fclose(yyin);
    yylex_destroy();

    printf("optimization result of .ll file: %d\n", optimize_llvm(ll_file));

    return 0;
}