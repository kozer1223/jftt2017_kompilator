#ifndef EXPRESSION_HH
#define EXPRESSION_HH

#include <ostream>
#include "Value.hh"

enum ExprOp {
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
  Value *lhs;
  Value *rhs;

  Expression(ExprOp, Value*, Value*);
  ~Expression();
};

#include <iostream>

inline Expression::Expression(ExprOp op, Value *l, Value *r){
  operation = op;
  lhs = l;
  rhs = r;
}

inline Expression::~Expression(){
  delete lhs;
  delete rhs;
}

inline std::ostream& operator<<(std::ostream &strm, const Expression &a) {
  strm << *(a.lhs);
  if (a.operation != ExprOp::Nop) {
    switch(a.operation){
      case ExprOp::Add : strm << " + "; break;
      case ExprOp::Sub : strm << " - "; break;
      case ExprOp::Mul : strm << " * "; break;
      case ExprOp::Div : strm << " / "; break;
      case ExprOp::Mod : strm << " % "; break;
    }
    strm << *(a.rhs);
  }
  return strm;
}

#endif
