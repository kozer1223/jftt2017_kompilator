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
  union{
    Identifier* identifier;
    mpz_class* number;
  } value;

  Value(Identifier*);
  Value(mpz_class);
  ~Value();
};

inline Value::Value(Identifier* id){
  type = ValueType::VIdentifier;
  value.identifier = id;
}

inline Value::Value(mpz_class num){
  type = ValueType::VConstant;
  mpz_class* number = new mpz_class(num);
  value.number = number;
}

inline Value::~Value(){
  if(type == ValueType::VIdentifier){
    delete value.identifier;
  } else if (type == ValueType::VConstant){
    delete value.number;
  }
}

inline std::ostream& operator<<(std::ostream &strm, const Value &a) {
  if(a.type == ValueType::VIdentifier){
    return strm << a.value.identifier->symbol;
  } else if (a.type == ValueType::VConstant){
    return strm << "CONST(" << *(a.value.number) << ")";
  }
  return strm;
}

#endif
