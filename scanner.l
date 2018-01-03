%{
#include <iostream>
#include <cstdlib>
#include "scanner.h"
#include "parser.hpp"
#include "CompilerDriver.hh"
#include "location.hh"
#include "Identifier.hh"
#include "Value.hh"
#include "Expression.hh"
#include "Condition.hh"
#include "Command.hh"
#include "CommandSet.hh"
#include "CommandBlock.hh"

using namespace std;

#define yyterminate() Compiler::Parser::make_SCANNEREND(Compiler::location());

#define YY_USER_ACTION driver.setLocation(yyleng);

%}
%option c++
%option nodefault
%option noyywrap
%option yyclass="Scanner"
%option prefix="Compiler_"
%x COMMENT
%%
"(" { BEGIN(COMMENT); }
[0-9]+ 	{ return Compiler::Parser::make_NUMBER(yytext, Compiler::location()); }
[_a-z]+ { return Compiler::Parser::make_PIDENTIFIER(yytext, Compiler::location()); }

"VAR"   { return Compiler::Parser::make_VAR(Compiler::location()); }
"BEGIN" { return Compiler::Parser::make_PBEGIN(Compiler::location()); }
"END"   { return Compiler::Parser::make_END(Compiler::location()); }

"IF"    { return Compiler::Parser::make_IF(Compiler::location()); }
"THEN"  { return Compiler::Parser::make_THEN(Compiler::location()); }
"ELSE"  { return Compiler::Parser::make_ELSE(Compiler::location()); }
"ENDIF" { return Compiler::Parser::make_ENDIF(Compiler::location()); }

"WHILE" { return Compiler::Parser::make_WHILE(Compiler::location()); }
"DO"    { return Compiler::Parser::make_DO(Compiler::location()); }
"ENDWHILE"  { return Compiler::Parser::make_ENDWHILE(Compiler::location()); }

"FOR"   { return Compiler::Parser::make_FOR(Compiler::location()); }
"FROM"  { return Compiler::Parser::make_FROM(Compiler::location()); }
"TO"    { return Compiler::Parser::make_TO(Compiler::location()); }
"DOWNTO"  { return Compiler::Parser::make_DOWNTO(Compiler::location()); }
"ENDFOR"  { return Compiler::Parser::make_ENDFOR(Compiler::location()); }

"READ"  { return Compiler::Parser::make_READ(Compiler::location()); }
"WRITE" { return Compiler::Parser::make_WRITE(Compiler::location()); }

"["   { return Compiler::Parser::make_LBRACKET(Compiler::location()); }
"]"   { return Compiler::Parser::make_RBRACKET(Compiler::location()); }

"+"	 { return Compiler::Parser::make_OPERATOR_PLUS(Compiler::location()); }
"-"	 { return Compiler::Parser::make_OPERATOR_MINUS(Compiler::location()); }
"*"	 { return Compiler::Parser::make_OPERATOR_MULT(Compiler::location()); }
"/"	 { return Compiler::Parser::make_OPERATOR_DIV(Compiler::location()); }
"%"	 { return Compiler::Parser::make_OPERATOR_MOD(Compiler::location()); }
":=" { return Compiler::Parser::make_OPERATOR_ASSIGN(Compiler::location()); }

"="  { return Compiler::Parser::make_COND_EQ(Compiler::location()); }
"<>" { return Compiler::Parser::make_COND_NEQ(Compiler::location()); }
"<"  { return Compiler::Parser::make_COND_LESS(Compiler::location()); }
">"  { return Compiler::Parser::make_COND_GREATER(Compiler::location()); }
"<=" { return Compiler::Parser::make_COND_LEQ(Compiler::location()); }
">=" { return Compiler::Parser::make_COND_GEQ(Compiler::location()); }

";"  { return Compiler::Parser::make_SEMICOLON(Compiler::location()); }
[ \n\t\r] {}
.	{
    return Compiler::Parser::make_ERROR(Compiler::location());
  }

<<EOF>>     { return yyterminate(); }

<COMMENT>")" { BEGIN(INITIAL); }
<COMMENT>\n {}
<COMMENT>. {}
<COMMENT><<EOF>>     { return yyterminate(); }
%%
