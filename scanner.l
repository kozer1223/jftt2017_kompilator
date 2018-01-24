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

#define yyterminate() Compiler::Parser::make_SCANNEREND(*loc);

#define YY_USER_ACTION loc->columns(yyleng);
%}
%option c++
%option nodefault
%option noyywrap
%option yyclass="Scanner"
%option prefix="Compiler_"
%x COMMENT
%%
"(" { BEGIN(COMMENT); }
[0-9]+ 	{ return Compiler::Parser::make_NUMBER(yytext, *loc); }
[_a-z]+ { return Compiler::Parser::make_PIDENTIFIER(yytext, *loc); }

"VAR"   { return Compiler::Parser::make_VAR(*loc); }
"BEGIN" { return Compiler::Parser::make_PBEGIN(*loc); }
"END"   { return Compiler::Parser::make_END(*loc); }

"IF"    { return Compiler::Parser::make_IF(*loc); }
"THEN"  { return Compiler::Parser::make_THEN(*loc); }
"ELSE"  { return Compiler::Parser::make_ELSE(*loc); }
"ENDIF" { return Compiler::Parser::make_ENDIF(*loc); }

"WHILE" { return Compiler::Parser::make_WHILE(*loc); }
"DO"    { return Compiler::Parser::make_DO(*loc); }
"ENDWHILE"  { return Compiler::Parser::make_ENDWHILE(*loc); }

"FOR"   { return Compiler::Parser::make_FOR(*loc); }
"FROM"  { return Compiler::Parser::make_FROM(*loc); }
"TO"    { return Compiler::Parser::make_TO(*loc); }
"DOWNTO"  { return Compiler::Parser::make_DOWNTO(*loc); }
"ENDFOR"  { return Compiler::Parser::make_ENDFOR(*loc); }

"READ"  { return Compiler::Parser::make_READ(*loc); }
"WRITE" { return Compiler::Parser::make_WRITE(*loc); }

"["   { return Compiler::Parser::make_LBRACKET(*loc); }
"]"   { return Compiler::Parser::make_RBRACKET(*loc); }

"+"	 { return Compiler::Parser::make_OPERATOR_PLUS(*loc); }
"-"	 { return Compiler::Parser::make_OPERATOR_MINUS(*loc); }
"*"	 { return Compiler::Parser::make_OPERATOR_MULT(*loc); }
"/"	 { return Compiler::Parser::make_OPERATOR_DIV(*loc); }
"%"	 { return Compiler::Parser::make_OPERATOR_MOD(*loc); }
":=" { return Compiler::Parser::make_OPERATOR_ASSIGN(*loc); }

"="  { return Compiler::Parser::make_COND_EQ(*loc); }
"<>" { return Compiler::Parser::make_COND_NEQ(*loc); }
"<"  { return Compiler::Parser::make_COND_LESS(*loc); }
">"  { return Compiler::Parser::make_COND_GREATER(*loc); }
"<=" { return Compiler::Parser::make_COND_LEQ(*loc); }
">=" { return Compiler::Parser::make_COND_GEQ(*loc); }

";"  { return Compiler::Parser::make_SEMICOLON(*loc); }
\n   { loc->lines(yyleng); loc->step(); }
[ \n\t\r] { loc->step(); }
.	{
    return Compiler::Parser::make_ERROR(*loc);
  }

<COMMENT>")" { BEGIN(INITIAL); }
<COMMENT>\n {}
<COMMENT>. {}
<COMMENT><<EOF>>     { return Compiler::Parser::make_ERROR(*loc); }

<<EOF>>     { return yyterminate(); }
%%
