#ifndef SYMBOLTABLE_HH
#define SYMBOLTABLE_HH

#include <string.h>
#include <gmp.h>
#include <gmpxx.h>
#include <map>
#include <vector>
#include <set>

struct Symbol {
  std::string name;
  mpz_class address;
  bool isArray;
  mpz_class arraySize;
};

class SymbolTable {
  std::map<std::string, Symbol> symbol_map;
  std::vector<std::string> symbol_list;
  std::set<std::string> iterator_list;
  std::set<std::string> temp_symbols;
  std::vector<std::string> iterator_stack;
  mpz_class lastFreeAddress;
  long long int lastTempSymbol;

public:
  SymbolTable();
  //~SymbolTable();

  void allocateSymbols();
  void allocateIterators();

  bool addSymbol(std::string symbol);
  bool addArraySymbol(std::string symbol, mpz_class size);
  bool addConstant(mpz_class constant);

  std::string addTempSymbol();

  bool pushIterator(std::string symbol);
  std::string popIterator();

  mpz_class getSymbol(std::string symbol);
  mpz_class getArraySize(std::string symbol);
  mpz_class getConstant(mpz_class constant);

  std::vector<std::string> getSymbols();
  std::set<std::string> getIterators();

  bool containsSymbol(std::string symbol);
  bool containsConstant(mpz_class constant);
  bool containsIterator(std::string symbol);
  bool iteratorOnStack(std::string symbol);

  bool isArray(std::string symbol);

  // debug
  void printSymbolData(std::string symbol);
};

#endif
