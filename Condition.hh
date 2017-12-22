#ifndef CONDITION_HH
#define CONDITION_HH

#include <ostream>
#include <vector>
#include <string>
#include "Value.hh"
#include "Command.hh"
#include "CommandBlock.hh"
#include "SymbolTable.hh"
#include "LabelManager.hh"

enum CondType {
  Equal,
  NotEqual,
  Less,
  Greater,
  LessEqual,
  GreaterEqual
};

class Condition{
public:
  CondType condition;
  Value lhs;
  Value rhs;

  Condition(): lhs(Value(0)), rhs(Value(0)) {}
  Condition(CondType cond, Value l, Value r): lhs(l), rhs(r){
    condition = cond;
  }
  ~Condition();

  std::vector<Command> getCommandBlock(SymbolTable* st, LabelManager* lm, CommandBlock& trueBlock);
  std::vector<Command> getCommandBlock(SymbolTable* st, LabelManager* lm, CommandBlock& trueBlock, CommandBlock& falseBlock);
};

inline Condition::~Condition(){}

inline std::ostream& operator<<(std::ostream &strm, const Condition &a) {
  strm << a.lhs;
  switch(a.condition){
    case CondType::Equal : strm << " = "; break;
    case CondType::NotEqual : strm << " <> "; break;
    case CondType::Less : strm << " < "; break;
    case CondType::Greater : strm << " > "; break;
    case CondType::LessEqual : strm << " <= "; break;
    case CondType::GreaterEqual : strm << " >= "; break;
  }
  strm << a.rhs;
  return strm;
}


#endif
