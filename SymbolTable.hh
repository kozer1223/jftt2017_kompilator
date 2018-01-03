#ifndef SYMBOLTABLE_HH
#define SYMBOLTABLE_HH

#include <string.h>
#include <gmp.h>
#include <gmpxx.h>
#include <map>
#include <vector>
#include <set>
#include "AssemblyCode.hh"

struct Symbol {
  std::string name;
  mpz_class address;
  bool isArray;
  mpz_class arraySize;
  bool initialized;
};

class SymbolTable {
  std::map<std::string, Symbol> symbol_map;
  std::map<mpz_class, Symbol> constants_map;
  std::vector<std::string> symbol_list;
  std::set<std::string> iterator_list;
  std::set<std::string> temp_symbols;
  std::set<mpz_class> constants;
  std::vector<std::string> iterator_stack;
  mpz_class lastFreeAddress;
  long long int lastTempSymbol;

public:
  mpz_class MUL_TEMP1 = 1;
  mpz_class MUL_TEMP2 = 2;
  mpz_class DIV_TEMP = 3;
  mpz_class DIV_REMAINDER = 4;
  mpz_class DIV_COPY = 5;
  mpz_class MOD_POWER = 6;


  SymbolTable();
  //~SymbolTable();

  void allocateSymbols();
  void allocateTempSymbols();
  void allocateIterators();
  void allocateConstants();

  bool addSymbol(std::string symbol);
  bool addArraySymbol(std::string symbol, mpz_class size);
  bool addConstant(mpz_class constant);
  bool addConstants(std::set<mpz_class>);

  std::string addTempSymbol();

  bool pushIterator(std::string symbol);
  std::string popIterator();

  mpz_class getSymbol(std::string symbol);
  mpz_class getArrayAddress(std::string symbol);
  mpz_class getArraySize(std::string symbol);
  mpz_class getConstant(mpz_class constant);

  std::vector<std::string> getSymbols();
  std::set<std::string> getIterators();

  bool containsSymbol(std::string symbol);
  bool containsConstant(mpz_class constant);
  bool containsIterator(std::string symbol);
  bool iteratorOnStack(std::string symbol);

  void initialize(std::string symbol);

  bool isInitialized(std::string symbol);
  bool isArray(std::string symbol);

  AssemblyCode constantCode(mpz_class numver);
  AssemblyCode generateConstantsCode();

  // debug
  void printSymbolData(std::string symbol);
};

#endif
