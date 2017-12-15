%option noyywrap
%option yylineno
%{
#include <cstdlib>
#include <cmath>
#include <string>
#include "Identifier.hh"
#include "Value.hh"
#include "Expression.hh"
#include "Condition.hh"
#include "Command.hh"
#include "CommandSet.hh"
#include "CommandBlock.hh"
#include "compiler.tab.h"
using namespace std;
int yylex();
%}
%x COMMENT
%%
"(" { BEGIN(COMMENT); }
[0-9]+ 	{ yylval.numval = strdup(yytext); return NUMBER; }
[_a-z]+ { yylval.strval = strdup(yytext); return PIDENTIFIER; }

"VAR"   { return VAR; }
"BEGIN" { return PBEGIN; }
"END"   { return END; }

"IF"    { return IF; }
"THEN"  { return THEN; }
"ELSE"  { return ELSE; }
"ENDIF" { return ENDIF; }

"WHILE" { return WHILE; }
"DO"    { return DO; }
"ENDWHILE"  { return ENDWHILE; }

"FOR"   { return FOR; }
"FROM"  { return FROM; }
"TO"    { return TO; }
"DOWNTO"  { return DOWNTO; }
"ENDFOR"  { return ENDFOR; }

"READ"  { return READ; }
"WRITE" { return WRITE; }

"["   { return LBRACKET; }
"]"   { return RBRACKET; }

"+"	 { return OPERATOR_PLUS; }
"-"	 { return OPERATOR_MINUS; }
"*"	 { return OPERATOR_MULT; }
"/"	 { return OPERATOR_DIV; }
"%"	 { return OPERATOR_MOD; }
":=" { return OPERATOR_ASSIGN; }

"="  { return COND_EQ; }
"<>" { return COND_NEQ; }
"<"  { return COND_LESS; }
">"  { return COND_GREATER; }
"<=" { return COND_LEQ; }
">=" { return COND_GEQ; }

";"  { return SEMICOLON; }
\n {}
.	{}

<COMMENT>")" { BEGIN(INITIAL); }
<COMMENT>\n {}
<COMMENT>. {}
%%
