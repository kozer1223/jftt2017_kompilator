%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0"
%defines
%define parser_class_name { Parser }
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.namespace { Compiler }
%code requires
{
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <set>
#include <algorithm>
#include <gmp.h>
#include <gmpxx.h>
#include "SymbolTable.hh"
#include "LabelManager.hh"
#include "Identifier.hh"
#include "Value.hh"
#include "Expression.hh"
#include "Condition.hh"
#include "Command.hh"
#include "CommandSet.hh"
#include "CommandBlock.hh"

using namespace std;

namespace Compiler {
	class Scanner;
	class CompilerDriver;
}

}

%code top
{
#include <iostream>
#include "scanner.h"
#include "parser.hpp"
#include "CompilerDriver.hh"
#include "location.hh"
#include <string>
#include <vector>
#include <stdint.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <set>
#include <algorithm>
#include <gmp.h>
#include <gmpxx.h>
#include "SymbolTable.hh"
#include "LabelManager.hh"
#include "Identifier.hh"
#include "Value.hh"
#include "Expression.hh"
#include "Condition.hh"
#include "Command.hh"
#include "CommandSet.hh"
#include "CommandBlock.hh"

static Compiler::Parser::symbol_type yylex(Compiler::Scanner &scanner, Compiler::CompilerDriver &driver) {
	return scanner.get_next_token();
}

using namespace Compiler;
}

%lex-param { Compiler::Scanner &scanner }
%lex-param { Compiler::CompilerDriver &driver }
%parse-param { Compiler::Scanner &scanner }
%parse-param { Compiler::CompilerDriver &driver }
%locations
%define parse.trace
%define parse.error verbose

%define api.token.prefix {TOKEN_}

%type <Identifier> identifier
%type <Value> value
%type <Expression> expression
%type <Condition> condition
%type <CommandSet> command
%type <CommandBlock> commands

%token SCANNEREND 0
%token ERROR

%token <std::string> NUMBER
%token <std::string> PIDENTIFIER

%token VAR
%token PBEGIN
%token END

%token IF
%token THEN
%token ELSE
%token ENDIF

%token WHILE
%token DO
%token ENDWHILE

%token FOR
%token FROM
%token TO
%token DOWNTO
%token ENDFOR

%token READ
%token WRITE

%token LBRACKET
%token RBRACKET

%token OPERATOR_PLUS
%token OPERATOR_MINUS
%token OPERATOR_MULT
%token OPERATOR_DIV
%token OPERATOR_MOD
%token OPERATOR_ASSIGN

%token COND_EQ
%token COND_NEQ
%token COND_LESS
%token COND_GREATER
%token COND_LEQ
%token COND_GEQ

%token SEMICOLON

%start program
%%
program: /*empty*/
| VAR vdeclarations
{
    driver.symbolTable.allocateSymbols();
    for(auto symbol : driver.symbolTable.getSymbols()){
      driver.symbolTable.printSymbolData(symbol);
    }
}
PBEGIN commands END
{
	driver.program = $5;
	driver.compile();
}
;
vdeclarations: /*empty*/
| vdeclarations PIDENTIFIER
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  }
  driver.symbolTable.addSymbol($2);
  //driver.symbolTable.printSymbolData($2);
}
| vdeclarations PIDENTIFIER LBRACKET NUMBER RBRACKET
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  }
	mpz_class size($4);
  driver.symbolTable.addArraySymbol($2, size);
  //driver.symbolTable.printSymbolData($2);
}
;
commands:
  commands command
{
	//cout << "new" << endl;
  //cout << $1 << endl;
  $1.pushCommandSet($2);
	$$ = $1;
  //cout << "INSERTED" << endl;
  //cout << $2 << endl;
  //delete $2;
}
| command
{
  $$.globalSymbolTable = &driver.symbolTable;
  $$.pushCommandSet($1);
  //delete $1;
}
;
command:
  identifier OPERATOR_ASSIGN expression SEMICOLON
{
  Identifier lhs = $1;
  if (driver.symbolTable.iteratorOnStack(lhs.symbol)){
    // assignment to iterator
      stringstream errMessage;
      errMessage << "Cannot assign value to iterator " << lhs.symbol << ".";
			Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  }
	if (!lhs.isArray){
		driver.symbolTable.initialize(lhs.symbol);
	}
  Expression rhs = $3;
  // convert assignment to appropriate command
  $$ = CommandSet(Command(lhs, rhs));
}
| IF condition THEN commands ELSE commands ENDIF
{
  std::vector<Command> conditionCommands;
  //cout << "PRE IF" << endl;
  conditionCommands = $2.getCommandBlock(&driver.symbolTable, &driver.labelManager, $4, $6);
  //cout << "CREATED COMMANDS" << endl;
  //delete $2;
  $$ = CommandSet(conditionCommands);
  //cout << "CREATED COMMANDSET" << endl;
  //cout << *$$ << endl;
}
| IF condition THEN commands ENDIF
{
  std::vector<Command> conditionCommands;
  conditionCommands = $2.getCommandBlock(&driver.symbolTable, &driver.labelManager, $4);
  //delete $2;
  $$ = CommandSet(conditionCommands);
}
| WHILE condition DO commands ENDWHILE
{
  std::vector<Command> conditionCommands;
	string loopLabel = driver.labelManager.nextLabel("loop");
	$4.pushCommand(Command(CommandType::Jump, new AddrLabel(loopLabel)));
  conditionCommands = $2.getCommandBlock(&driver.symbolTable, &driver.labelManager, $4);
	conditionCommands.insert(conditionCommands.begin(), Command(CommandType::Label, new AddrLabel(loopLabel)));
  //delete $2;
  $$ = CommandSet(conditionCommands);
}
| FOR PIDENTIFIER FROM value TO value
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists.";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  } else if (driver.symbolTable.iteratorOnStack($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " is already used as an iterator.";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  }
  driver.symbolTable.pushIterator($2);
}
DO commands ENDFOR
{
  driver.symbolTable.popIterator();
  //$$ = nullptr;
}
| FOR PIDENTIFIER FROM value DOWNTO value
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists.";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  } else if (driver.symbolTable.iteratorOnStack($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " is already used as an iterator.";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  }
  driver.symbolTable.pushIterator($2);
}
DO commands ENDFOR
{
  driver.symbolTable.popIterator();
  //$$ = nullptr;
}
| READ identifier SEMICOLON
{
	if (!$2.isArray){
		driver.symbolTable.initialize($2.symbol);
	}
  $$ = CommandSet(Command(CommandType::Read, $2));
}
| WRITE value SEMICOLON
{
  $$ = CommandSet(Command(CommandType::Write, $2));
}
;
expression:
  value
{
  $$ = Expression(ExprOp::Nop, $1, $1);
}
| value OPERATOR_PLUS value
{
  $$ = Expression(ExprOp::Add, $1, $3);
}
| value OPERATOR_MINUS value
{
  $$ = Expression(ExprOp::Sub, $1, $3);
}
| value OPERATOR_MULT value
{
  $$ = Expression(ExprOp::Mul, $1, $3);;
}
| value OPERATOR_DIV value
{
  $$ = Expression(ExprOp::Div, $1, $3);
}
| value OPERATOR_MOD value
{
  $$ = Expression(ExprOp::Mod, $1, $3);
}
;
condition:
  value COND_EQ value
{
  $$ = Condition(CondType::Equal, $1, $3);
}
| value COND_NEQ value
{
  $$ = Condition(CondType::NotEqual, $1, $3);
}
| value COND_LESS value
{
  $$ = Condition(CondType::Less, $1, $3);
}
| value COND_GREATER value
{
  $$ = Condition(CondType::Greater, $1, $3);
}
| value COND_LEQ value
{
  $$ = Condition(CondType::LessEqual, $1, $3);
}
| value COND_GEQ value
{
  $$ = Condition(CondType::GreaterEqual, $1, $3);
}
;
value:
  NUMBER      { mpz_class number($1); $$ = Value(number); }
| identifier
{
	if (!$1.isArray && !driver.symbolTable.isInitialized($1.symbol)){
    stringstream errMessage;
    errMessage << "Variable " << $1 << " was not initialized";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	}
	$$ = Value($1);
}
;
identifier:
  PIDENTIFIER
{
  if (!driver.symbolTable.containsSymbol($1) && !driver.symbolTable.iteratorOnStack($1)){
    stringstream errMessage;
    errMessage << "Variable " << $1 << " was not declared";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
  } else if (driver.symbolTable.isArray($1)){
		stringstream errMessage;
    errMessage << "Variable " << $1 << " is an array";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	}
  $$ = Identifier($1);
}
| PIDENTIFIER LBRACKET PIDENTIFIER RBRACKET
{
	if (!driver.symbolTable.containsSymbol($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " was not declared";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	} else if (!driver.symbolTable.containsSymbol($3)){
		stringstream errMessage;
		errMessage << "Variable " << $3 << " was not declared";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	}  else if (!driver.symbolTable.isArray($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " is not an array";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	} else if (driver.symbolTable.isArray($3)){
		stringstream errMessage;
		errMessage << "Variable " << $3 << " is an array";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	}
	$$ = Identifier($1, $3);
}
| PIDENTIFIER LBRACKET NUMBER RBRACKET
{
	mpz_class index($3);
	if (!driver.symbolTable.containsSymbol($1) && !driver.symbolTable.iteratorOnStack($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " was not declared";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	} else if (!driver.symbolTable.isArray($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " is not an array";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	} else if (index >= driver.symbolTable.getArraySize($1)){
		stringstream errMessage;
		errMessage << "Index " << $3 << " out of array bounds";
		Compiler::Parser::error(Compiler::location(), errMessage.str()); return 1;
	}
	$$ = Identifier($1, index);
}
;
%%

void Compiler::Parser::error(const location &loc , const std::string &message) {
	cout << "Error at " << loc << ":" << endl << message << endl;
}
