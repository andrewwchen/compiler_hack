%{
#include<stdio.h>
#include<cstddef>
#include<vector>
#include"ast.h"
extern int yylex();
extern int yylex_destroy();
extern int yywrap();
int yyerror(char *);
extern FILE * yyin;
%}
%union{
	int ival;
	char *sname;
}

%token EXTERN
%token VOID
%token INT
%token PRINT
%token READ
%token IF
%token ELSE
%token WHILE
%token RETURN
%token EQ
%token GEQ
%token LEQ
%token <ival> NUM 
%token <sname> NAME

%nonassoc IFX
%nonassoc ELSE

%start prog
%%
prog : extern_print extern_read func {createProg}
extern_print : EXTERN VOID PRINT '(' INT ')' ';' {printf("define extern print\n");}
extern_read : EXTERN INT READ '(' ')' ';' {printf("define extern read\n");}
func : INT NAME '(' INT NAME ')' stmt {printf("define function %s \n", $2);}
		| INT NAME '(' ')' stmt {printf("define function %s \n", $2);}
stmt : call
		| ret
		| block
		| whilen
		| ifn
		| decl
		| asgn
val : read_call
	| name_num '+' name_num
	| name_num '-' name_num
	| name_num '/' name_num
	| name_num '*' name_num
	| name_num
name_num : NAME
		| NUM
cond : val '>' val {printf("greater than\n");}
		| val '<' val {printf("less than\n");}
		| val LEQ val {printf("less than equal to\n");}
		| val GEQ val {printf("greater than equal to\n");}
		| val EQ val {printf("equal to\n");}
call : print_call ';'
		| read_call ';'
print_call : PRINT '(' val ')' {printf("print call\n");}
read_call : READ '(' ')' {printf("read call\n");}
ret : RETURN '(' val ')' ';' {printf("return statement\n");}
		| RETURN val ';'
block : '{' stmts '}' {printf("block statement\n");}
stmts : stmts stmt
		| stmt
whilen : WHILE '(' cond ')' stmt {printf("while statement\n");}
ifn : IF '(' cond ')' stmt %prec IFX {printf("if statement\n");} 
		| IF '(' cond ')' stmt ELSE stmt {printf("if else statement\n");}
decl : INT NAME ';' {printf("declare variable %s\n", $2);}
asgn : NAME '=' val ';' {printf("assign variable\n");}

%%
int yyerror(char *s){
	fprintf(stderr,"%s\n", s);
	return 0;
}

int main(int argc, char* argv[]){
		if (argc == 2){
			yyin = fopen(argv[1], "r");
			if (yyin == NULL) {
				fprintf(stderr, "File open error\n");
				return 1;
			}
		}
		yyparse();
		if (argc == 2) fclose(yyin);
		yylex_destroy();
		return 0;
}
