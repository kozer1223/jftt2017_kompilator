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

  Identifier(std::string symbol);
};

inline Identifier::Identifier(std::string symbol){
  this->symbol = symbol;
  isArray = false;
}

inline std::ostream& operator<<(std::ostream &strm, const Identifier &a) {
  return strm << a.symbol;
}

#endif
