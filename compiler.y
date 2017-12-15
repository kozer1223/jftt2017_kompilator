%error-verbose
%{
#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
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

extern int yylineno;
static SymbolTable *symbolTable = new SymbolTable();
static LabelManager *labelManager = new LabelManager();

int yylex();
int yyerror(const std::string& error);
%}
%union {
	char* numval;
	char* strval;
  Identifier* identifier;
  Value* value;
  Expression* expression;
  Condition* condition;
  CommandSet* commandset;
  CommandBlock* block;
}

%type <identifier> identifier
%type <value> value
%type <expression> expression
%type <condition> condition
%type <commandset> command
%type <block> commands

%token ERROR

%token <numval> NUMBER
%token <strval> PIDENTIFIER

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
%%
program: /*empty*/
| VAR vdeclarations
{
    symbolTable->allocateSymbols();
    for(auto symbol : symbolTable->getSymbols()){
      symbolTable->printSymbolData(symbol);
    }
}
PBEGIN commands END
{
  symbolTable->allocateIterators();
  for(auto symbol : symbolTable->getIterators()){
    symbolTable->printSymbolData(symbol);
  }
  CommandBlock* programBlock = $5;
  // push halt as final instruction
  programBlock->pushCommand(new Command(CommandType::Halt, nullptr, nullptr, nullptr));
  // convert symbols and values to tri-address form
  programBlock->convertToTriAddress();
  cout << (*programBlock);
}
;
vdeclarations: /*empty*/
| vdeclarations PIDENTIFIER
{
  if (symbolTable->containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists";
    yyerror(errMessage.str());
    return ERROR;
  }
  symbolTable->addSymbol($2);
  symbolTable->printSymbolData($2);
}
| vdeclarations PIDENTIFIER LBRACKET NUMBER RBRACKET
;
commands:
  commands command
{
  //cout << *$1 << endl;
  $1->pushCommandSet($2);
  //cout << "INSERTEDF" << endl;
  //delete $2;
}
| command
{
  $$ = new CommandBlock(symbolTable);
  $$->pushCommandSet($1);
  //delete $1;
}
;
command:
  identifier OPERATOR_ASSIGN expression SEMICOLON
{
  Identifier *lhs = $1;
  if (symbolTable->iteratorOnStack(lhs->symbol)){
    // assignment to iterator
      stringstream errMessage;
      errMessage << "Cannot assign value to iterator " << lhs->symbol << ".";
      yyerror(errMessage.str());
      return ERROR;
  }
  Expression *rhs = $3;
  // convert assignment to appropriate command
  $$ = new CommandSet(new Command(lhs, rhs));
}
| IF condition THEN commands ELSE commands ENDIF
{
  std::vector<Command*> conditionCommands;
  //cout << "PRE IF" << endl;
  conditionCommands = $2->getCommandBlock(symbolTable, labelManager, $4, $6);
  //cout << "CREATED COMMANDS" << endl;
  //delete $2;
  $$ = new CommandSet(conditionCommands);
  //cout << "CREATED COMMANDSET" << endl;
  //cout << *$$ << endl;
}
| IF condition THEN commands ENDIF
{
  std::vector<Command*> conditionCommands;
  conditionCommands = $2->getCommandBlock(symbolTable, labelManager, $4);
  //delete $2;
  $$ = new CommandSet(conditionCommands);
}
| WHILE condition DO commands ENDWHILE
{
  $$ = nullptr;
}
| FOR PIDENTIFIER FROM value TO value
{
  if (symbolTable->containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists.";
    yyerror(errMessage.str());
    return ERROR;
  } else if (symbolTable->iteratorOnStack($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " is already used as an iterator.";
    yyerror(errMessage.str());
    return ERROR;
  }
  symbolTable->pushIterator($2);
}
DO commands ENDFOR
{
  symbolTable->popIterator();
  $$ = nullptr;
}
| FOR PIDENTIFIER FROM value DOWNTO value
{
  if (symbolTable->containsSymbol($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " already exists.";
    yyerror(errMessage.str());
    return ERROR;
  } else if (symbolTable->iteratorOnStack($2)){
    stringstream errMessage;
    errMessage << "Identifier " << $2 << " is already used as an iterator.";
    yyerror(errMessage.str());
    return ERROR;
  }
  symbolTable->pushIterator($2);
}
DO commands ENDFOR
{
  symbolTable->popIterator();
  $$ = nullptr;
}
| READ identifier SEMICOLON
{
  $$ = new CommandSet(new Command(CommandType::Read, $2));
}
| WRITE value SEMICOLON
{
  $$ = new CommandSet(new Command(CommandType::Write, $2));
}
;
expression:
  value
{
  $$ = new Expression(ExprOp::Nop, $1, nullptr);
}
| value OPERATOR_PLUS value
{
  $$ = new Expression(ExprOp::Add, $1, $3);
}
| value OPERATOR_MINUS value
{
  $$ = new Expression(ExprOp::Sub, $1, $3);
}
| value OPERATOR_MULT value
{
  $$ = new Expression(ExprOp::Mul, $1, $3);;
}
| value OPERATOR_DIV value
{
  $$ = new Expression(ExprOp::Div, $1, $3);
}
| value OPERATOR_MOD value
{
  $$ = new Expression(ExprOp::Mod, $1, $3);
}
;
condition:
  value COND_EQ value
{
  $$ = new Condition(CondType::Equal, $1, $3);
}
| value COND_NEQ value
{
  $$ = new Condition(CondType::NotEqual, $1, $3);
}
| value COND_LESS value
{
  $$ = new Condition(CondType::Less, $1, $3);
}
| value COND_GREATER value
{
  $$ = new Condition(CondType::Greater, $1, $3);
}
| value COND_LEQ value
{
  $$ = new Condition(CondType::LessEqual, $1, $3);
}
| value COND_GEQ value
{
  $$ = new Condition(CondType::GreaterEqual, $1, $3);
}
;
value:
  NUMBER      { mpz_class number($1); $$ = new Value(number); }
| identifier  { $$ = new Value($1); }
;
identifier:
  PIDENTIFIER
{
  if (!symbolTable->containsSymbol($1) && !symbolTable->iteratorOnStack($1)){
    stringstream errMessage;
    errMessage << "Variable " << $1 << " was not declared";
    yyerror(errMessage.str());
    return ERROR;
  }
  $$ = new Identifier($1);
}
| PIDENTIFIER LBRACKET PIDENTIFIER RBRACKET
| PIDENTIFIER LBRACKET NUMBER RBRACKET
;
%%

int yyerror(std::string const& error) {
		cout << "Error at line " << yylineno << ":" << endl << error.c_str() << endl;
		return 0;
}

int main() {
		yyparse();
		cout << "ok" << endl;

    // cleanup
    delete symbolTable;
		return 0;
}
