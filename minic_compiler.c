#include<stdio.h>
#include"ast.h"
#include"ir.h"
#include"semantic_analysis.h"
#include"llvm_parser.h"
#include"codegen.h"

extern int yylex_destroy();
extern int yyparse (astNode **rootAstPtr);
extern FILE * yyin;

int main(int argc, char** argv)
{
	if (argc != 3){
        printf("Usage: ./minic_compiler.out {.c file 1} {.c file 2}\n");
        return -1;
    }
    char* c_file1 = argv[1];
    char* c_file2 = argv[2];

    yyin = fopen(c_file1, "r");
    if (yyin == NULL) {
        fprintf(stderr, "File open error\n");
        yylex_destroy();
        return -1;
    }
    astNode *rootAst1;
    yyparse(&rootAst1);

    fclose(yyin);
    yylex_destroy();

    yyin = fopen(c_file2, "r");
    if (yyin == NULL) {
        fprintf(stderr, "File open error\n");
        yylex_destroy();
        return -1;
    }
    astNode *rootAst2;
    yyparse(&rootAst2);

    fclose(yyin);
    yylex_destroy();


    printf("static analysis result of .c file 1: %d\n", perform_analysis(rootAst1));
    printf("ir build result of .c file 1: %d\n", build_ir(rootAst1, c_file1));
    printf("static analysis result of .c file 2: %d\n", perform_analysis(rootAst1));
    printf("ir build result of .c file 2: %d\n", build_ir(rootAst1, c_file1));    
    //printNode(rootAst1, 1);
    //printNode(rootAst2, 1);
    int similarity_score = calculate_similarity(rootAst1, rootAst2);

    printf("similarity: %d\n", similarity_score);    
    freeNode(rootAst1);
    freeNode(rootAst2);

    return 0;
}