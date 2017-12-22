#ifndef VALUE_HH
#define VALUE_HH

#include <string.h>
#include <gmp.h>
#include <gmpxx.h>
#include <ostream>
#include "Identifier.hh"

enum ValueType{
  VIdentifier,
  VConstant
};

class Value{
public:
  ValueType type;
  Identifier identifier;
  mpz_class number;

  Value(): identifier(Identifier("")) {};
  Value(Identifier i): identifier(i){
    type = ValueType::VIdentifier;
    identifier = i;
  }
  Value(mpz_class num): identifier(Identifier("")){
    type = ValueType::VConstant;
    number = num;
    identifier = Identifier("");
  }
};

inline std::ostream& operator<<(std::ostream &strm, const Value &a) {
  if(a.type == ValueType::VIdentifier){
    return strm << a.identifier.symbol;
  } else if (a.type == ValueType::VConstant){
    return strm << "CONST(" << a.number << ")";
  }
  return strm;
}

#endif
