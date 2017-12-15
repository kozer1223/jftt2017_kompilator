%option noyywrap
%option yylineno
%{
#include "binarytree.h"
#include "calc.tab.h"
#include <cstdlib>
#include <cmath>
int yylex();
%}
%%
\#(.|\\\n)*\n	;
[ \t]+ 	;
\\\n	;
[0-9]+ 	{ yylval.node = newNode(VAL, atoi(yytext)); return VAL; }
"+"	{ return PLUS; }
"-"	{ return MINUS; }
"*"	{ return MULT; }
"/"	{ return DIV; }
"^"	{ return EXP; }
"("	{ return LPAR; }
")"	{ return RPAR; }
\n	{ return END; }
.	{ return ERROR; }
%%

