%{
#include<stdio.h>
#include<cstddef>
#include<vector>
#include"ast.h"
#include "y.tab.h"
using namespace std;
extern int yylex();
int yyerror(astNode** rootAstPtr, const char *);
%}
%union{
	int ival;
	char *sname;
	astNode *node;
	vector<astNode*> *nodes;
}
%parse-param {astNode **rootAstPtr}

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
%nonassoc NO_DEC
%nonassoc DEC
%nonassoc IFX
%nonassoc ELSE
%type <node> prog extern_print extern_read func var cnst var_cnst val cond stmt call print_call read_call ret block whilen ifn decl asgn
%type <nodes> stmts decls
%start prog
%%
prog : extern_print extern_read func {*rootAstPtr = createProg($1, $2, $3);}
extern_print : EXTERN VOID PRINT '(' INT ')' ';' {$$ = createExtern("print");}
extern_read : EXTERN INT READ '(' ')' ';' {$$ = createExtern("read");}
func : INT NAME '(' INT var ')' stmt {$$ = createFunc($2, $5, $7); free($2);}
	 | INT NAME '(' ')' stmt {$$ = createFunc($2, NULL, $5); free($2);}
var : NAME {$$ = createVar($1); free($1);}
cnst: NUM {$$ = createCnst($1);}
var_cnst : var {$$ = $1;}
		 | cnst {$$ = $1;}
		 | '-' var {$$ = createUExpr($2, uminus);}
		 | '-' cnst {$$ = createUExpr($2, uminus);}

val : read_call {$$ = $1;}
	| var_cnst '+' var_cnst {$$ = createBExpr($1, $3, add);}
	| var_cnst '-' var_cnst {$$ = createBExpr($1, $3, sub);}
	| var_cnst '/' var_cnst {$$ = createBExpr($1, $3, divide);}
	| var_cnst '*' var_cnst {$$ = createBExpr($1, $3, mul);}
	| var_cnst {$$ = $1;}
cond : val '>' val {$$ = createRExpr($1, $3, gt);}
		| val '<' val {$$ = createRExpr($1, $3, lt);}
		| val LEQ val {$$ = createRExpr($1, $3, le);}
		| val GEQ val {$$ = createRExpr($1, $3, ge);}
		| val EQ val {$$ = createRExpr($1, $3, eq);}
stmt : call {$$ = $1;}
		| ret {$$ = $1;}
		| block {$$ = $1;}
		| whilen {$$ = $1;}
		| ifn {$$ = $1;}
		| asgn {$$ = $1;}
call : print_call ';' {$$ = $1;}
		| read_call ';' {$$ = $1;}
print_call : PRINT '(' val ')' {$$ = createCall("print", $3);}
read_call : READ '(' ')' {$$ = createCall("read");}
ret : RETURN '(' val ')' ';' {$$ = createRet($3);}
		| RETURN val ';' {$$ = createRet($2);}
block : '{' decls stmts '}' {
	$2->insert($2->end(), $3->begin(), $3->end());
	delete($3);
	$$ = createBlock($2);
}
block : '{' stmts '}' {
	$$ = createBlock($2);
}
decls : decls decl {
			$1->push_back($2);
			$$ = $1;
		}
		| decl {
			vector<astNode*> *dlist;
			dlist = new vector<astNode*> ();
			dlist->push_back($1);
			$$ = dlist;
		}
stmts : stmts stmt {
			$1->push_back($2);
			$$ = $1;
		}
		| stmt {
			vector<astNode*> *slist;
			slist = new vector<astNode*> ();
			slist->push_back($1);
			$$ = slist;
		}
whilen : WHILE '(' cond ')' stmt {$$ = createWhile($3, $5);}
ifn : IF '(' cond ')' stmt %prec IFX {$$ = createIf($3, $5);}
		| IF '(' cond ')' stmt ELSE stmt {$$ = createIf($3, $5, $7);}
decl : INT NAME ';' {$$ = createDecl($2); free($2);}
asgn : var '=' val ';' {$$ = createAsgn($1, $3);}

%%
int yyerror(astNode** rootAstPtr, const char *s){
	fprintf(stderr,"%s\n", s);
	return 0;
}
