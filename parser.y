%skeleton "lalr1.cc" // C++
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
%initial-action
{

};
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
		// allocate all declared symbols
    driver.symbolTable.allocateSymbols();
		/* //DEBUG
    for(auto symbol : driver.symbolTable.getSymbols()){
      driver.symbolTable.printSymbolData(symbol);
    }*/
}
PBEGIN commands END
{
	driver.program = $5;
}
;
vdeclarations: /*empty*/
| vdeclarations PIDENTIFIER
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
  }
  driver.symbolTable.addSymbol($2);
}
| vdeclarations PIDENTIFIER LBRACKET NUMBER RBRACKET
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
  }
	mpz_class size($4);
  driver.symbolTable.addArraySymbol($2, size);
}
;
commands:
  commands command
{
  $1.pushCommandSet($2);
	$$ = $1;
}
| command
{
  $$.globalSymbolTable = &driver.symbolTable;
  $$.pushCommandSet($1);
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
			Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
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
  conditionCommands = $2.getCommandBlock(&driver.symbolTable, &driver.labelManager, $4, $6);
  $$ = CommandSet(conditionCommands);
}
| IF condition THEN commands ENDIF
{
  std::vector<Command> conditionCommands;
  conditionCommands = $2.getCommandBlock(&driver.symbolTable, &driver.labelManager, $4);
  $$ = CommandSet(conditionCommands);
}
| WHILE condition DO commands ENDWHILE
{
  std::vector<Command> conditionCommands;
	// generate while loop
	string loopLabel = driver.labelManager.nextLabel("loop");
	$4.pushCommand(Command(CommandType::Jump, new AddrLabel(loopLabel)));
  conditionCommands = $2.getCommandBlock(&driver.symbolTable, &driver.labelManager, $4);
	conditionCommands.insert(conditionCommands.begin(), Command(CommandType::Label, new AddrLabel(loopLabel)));
  $$ = CommandSet(conditionCommands);
}
| FOR PIDENTIFIER FROM value TO value
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists.";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
  } else if (driver.symbolTable.iteratorOnStack($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " is already used as an iterator.";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
  }
  driver.symbolTable.pushIterator($2);
}
DO commands ENDFOR
{
	// generate for loop
	std::vector<Command> preambleCommands;
	std::vector<Command> conditionCommands;

	// iterator = value1
	auto c1addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier($2)));
	auto c1addr2 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>($4));
	Command c1 = Command(CommandType::Assign, c1addr1, c1addr2);
	preambleCommands.push_back(c1);

  // tmp = value2
	std::string tempSymbol = driver.symbolTable.addTempSymbol();
	auto c2addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	auto c2addr2 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>($6));
	Command c2 = Command(CommandType::Assign, c2addr1, c2addr2);
	preambleCommands.push_back(c2);

 	// tmp ++
	auto c3addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	Command c3 = Command(CommandType::Increase, c3addr1);
	preambleCommands.push_back(c3);

	// tmp = tmp - value1
	auto c4addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	auto c4addr2 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	auto c4addr3 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>($4));
	Command c4 = Command(CommandType::AssignSub, c4addr1, c4addr2, c4addr3);
	preambleCommands.push_back(c4);

	Condition greater = Condition(CondType::Greater, Value(Identifier(tempSymbol)), Value((mpz_class)0));

	// while block
	string loopLabel = driver.labelManager.nextLabel("loop");

	// i++
	auto c5addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier($2)));
	$9.pushCommand(Command(CommandType::Increase, c5addr1));
	// temp--
	auto c6addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	$9.pushCommand(Command(CommandType::Decrease, c6addr1));

	$9.pushCommand(Command(CommandType::Jump, new AddrLabel(loopLabel)));
  conditionCommands = greater.getCommandBlock(&driver.symbolTable, &driver.labelManager, $9);
	conditionCommands.insert(conditionCommands.begin(), Command(CommandType::Label, new AddrLabel(loopLabel)));

	preambleCommands.insert(preambleCommands.end(), conditionCommands.begin(), conditionCommands.end());
  $$ = CommandSet(preambleCommands);

  driver.symbolTable.popIterator();
}
| FOR PIDENTIFIER FROM value DOWNTO value
{
  if (driver.symbolTable.containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists.";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
  } else if (driver.symbolTable.iteratorOnStack($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " is already used as an iterator.";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
  }
  driver.symbolTable.pushIterator($2);
}
DO commands ENDFOR
{
	// generate for loop
	std::vector<Command> preambleCommands;
	std::vector<Command> conditionCommands;

	// iterator = value1
	auto c1addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier($2)));
	auto c1addr2 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>($4));
	Command c1 = Command(CommandType::Assign, c1addr1, c1addr2);
	preambleCommands.push_back(c1);

  // tmp = value1
	std::string tempSymbol = driver.symbolTable.addTempSymbol();
	auto c2addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	auto c2addr2 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>($4));
	Command c2 = Command(CommandType::Assign, c2addr1, c2addr2);
	preambleCommands.push_back(c2);

 	// tmp ++
	auto c3addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	Command c3 = Command(CommandType::Increase, c3addr1);
	preambleCommands.push_back(c3);

	// tmp = tmp - value2
	auto c4addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	auto c4addr2 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	auto c4addr3 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>($6));
	Command c4 = Command(CommandType::AssignSub, c4addr1, c4addr2, c4addr3);
	preambleCommands.push_back(c4);

	Condition greater = Condition(CondType::Greater, Value(Identifier(tempSymbol)), Value((mpz_class)0));

	// while block
	string loopLabel = driver.labelManager.nextLabel("loop");

	// i--
	auto c5addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier($2)));
	$9.pushCommand(Command(CommandType::Decrease, c5addr1));
	// temp--
	auto c6addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(Identifier(tempSymbol)));
	$9.pushCommand(Command(CommandType::Decrease, c6addr1));

	$9.pushCommand(Command(CommandType::Jump, new AddrLabel(loopLabel)));
  conditionCommands = greater.getCommandBlock(&driver.symbolTable, &driver.labelManager, $9);
	conditionCommands.insert(conditionCommands.begin(), Command(CommandType::Label, new AddrLabel(loopLabel)));

	preambleCommands.insert(preambleCommands.end(), conditionCommands.begin(), conditionCommands.end());
  $$ = CommandSet(preambleCommands);

  driver.symbolTable.popIterator();
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
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
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
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
  } else if (driver.symbolTable.isArray($1)){
		stringstream errMessage;
    errMessage << "Variable " << $1 << " is an array";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	}
  $$ = Identifier($1);
}
| PIDENTIFIER LBRACKET PIDENTIFIER RBRACKET
{
	if (!driver.symbolTable.containsSymbol($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " was not declared";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	} else if (!driver.symbolTable.containsSymbol($3) && !driver.symbolTable.iteratorOnStack($3)){
		stringstream errMessage;
		errMessage << "Variable " << $3 << " was not declared";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	}  else if (!driver.symbolTable.isArray($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " is not an array";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	} else if (driver.symbolTable.isArray($3)){
		stringstream errMessage;
		errMessage << "Variable " << $3 << " is an array";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	} else if (!driver.symbolTable.isInitialized($3)){
		stringstream errMessage;
		errMessage << "Variable " << $3 << " was not initialized";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	}
	$$ = Identifier($1, $3);
}
| PIDENTIFIER LBRACKET NUMBER RBRACKET
{
	mpz_class index($3);
	if (!driver.symbolTable.containsSymbol($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " was not declared";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	} else if (!driver.symbolTable.isArray($1)){
		stringstream errMessage;
		errMessage << "Variable " << $1 << " is not an array";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	} else if (index >= driver.symbolTable.getArraySize($1)){
		stringstream errMessage;
		errMessage << "Index " << $3 << " out of array bounds";
		Compiler::Parser::error(*(scanner.loc), errMessage.str()); return 1;
	}
	$$ = Identifier($1, index);
}
;
%%

void Compiler::Parser::error(const location &loc , const std::string &message) {
	cerr << "Error at " << loc << ":" << endl << message << endl;
}
