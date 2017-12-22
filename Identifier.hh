#ifndef IDENTIFIER_HH
#define IDENTIFIER_HH

#include <string.h>
#include <gmp.h>
#include <gmpxx.h>

enum IndexType { Variable, Number };

class Identifier {
public:
  std::string symbol;
  bool isArray;
  IndexType indexType;
  std::string symbolIndex;
  mpz_class numberIndex;

  Identifier() {};
  Identifier(std::string symbol);
  Identifier(std::string symbol, mpz_class index);
  Identifier(std::string symbol, std::string index);
};

inline Identifier::Identifier(std::string s){
  symbol = s;
  isArray = false;
}

inline Identifier::Identifier(std::string s, mpz_class index){
  symbol = s;
  isArray = true;
  indexType = IndexType::Number;
  numberIndex = index;
}

inline Identifier::Identifier(std::string s, std::string index){
  symbol = s;
  isArray = true;
  indexType = IndexType::Variable;
  symbolIndex = index;
}

inline std::ostream& operator<<(std::ostream &strm, const Identifier &a) {
  if (a.isArray){
    if (a.indexType == IndexType::Number){
      return strm << a.symbol << "[" << a.numberIndex << "]";
    }
    return strm << a.symbol << "[" << a.symbolIndex << "]";
  } else {
    return strm << a.symbol;
  }
}

#endif
