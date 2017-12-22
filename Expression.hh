#ifndef EXPRESSION_HH
#define EXPRESSION_HH

#include <ostream>
#include "Value.hh"

enum class ExprOp {
  Nop,
  Add,
  Sub,
  Mul,
  Div,
  Mod
};

class Expression{
public:
  ExprOp operation;
  Value lhs;
  Value rhs;

  Expression(): lhs(Value(0)), rhs(Value(0)) {}
  Expression(ExprOp op, Value l, Value r): lhs(l), rhs(r){
    operation = op;
  }
  ~Expression();
};

#include <iostream>

inline Expression::~Expression(){}

inline std::ostream& operator<<(std::ostream &strm, const Expression &a) {
  strm << a.lhs;
  if (a.operation != ExprOp::Nop) {
    switch(a.operation){
      case ExprOp::Add : strm << " + "; break;
      case ExprOp::Sub : strm << " - "; break;
      case ExprOp::Mul : strm << " * "; break;
      case ExprOp::Div : strm << " / "; break;
      case ExprOp::Mod : strm << " % "; break;
      case ExprOp::Nop : break;
      default: break;
    }
    strm << a.rhs;
  }
  return strm;
}

#endif
