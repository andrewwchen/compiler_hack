%{
#include<stdio.h>
#include"ast.h"
#include "y.tab.h"
extern int yyerror(char *);
%}
%%
extern 	{ return EXTERN;}
void	{ return VOID;}
int		{ return INT;}
print	{ return PRINT;}
read	{ return READ;}
if		{ return IF;}
else	{ return ELSE;}
while	{ return WHILE;}
return	{ return RETURN;}
==		{ return EQ;}
>=		{ return GEQ;}
\<=		{ return LEQ;}
[(){};=+\-*><] { return yytext[0];}
[0-9]+	{ yylval.ival = atoi(yytext); 
		return NUM;}
[a-zA-Z][a-zA-Z0-9_]*	{ yylval.sname = (char *) calloc(strlen(yytext)+1, sizeof(char));
						strcpy(yylval.sname, yytext); 
						return NAME;}
[ \n\t] 		
.						{ return yytext[0];}
%%
int yywrap(){
	return 1;
}
