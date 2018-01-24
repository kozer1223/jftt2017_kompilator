#include "SymbolTable.hh"
#include <iostream>
#include <sstream>

SymbolTable::SymbolTable(){
  lastFreeAddress = FIRSTFREEADDRESS; // reserve space for temp variables
  lastTempSymbol = 1;
}

void SymbolTable::allocateSymbols(){
  lastFreeAddress = FIRSTFREEADDRESS;
  for (auto symbol : symbol_list) {
    struct Symbol& symbolData = symbol_map[symbol];
    symbolData.address = lastFreeAddress;
    if (symbolData.isArray){
      lastFreeAddress += symbolData.arraySize;
      // array address constant
      // addConstant(symbolData.address + 1);
    }
    lastFreeAddress++;
  }
}

void SymbolTable::allocateTempSymbols(){
  for (auto symbol : temp_symbols) {
    struct Symbol& symbolData = symbol_map[symbol];
    symbolData.address = lastFreeAddress;
    lastFreeAddress++;
  }
}

void SymbolTable::allocateIterators(){
  for (auto symbol : iterator_list) {
    struct Symbol& symbolData = symbol_map[symbol];
    symbolData.address = lastFreeAddress;
    lastFreeAddress++;
  }
}

void SymbolTable::allocateConstants(){
  for (auto constant : constants) {
    struct Symbol& symbolData = constants_map[constant];
    symbolData.address = lastFreeAddress;
    lastFreeAddress++;
  }
}

bool SymbolTable::addSymbol(std::string symbol){
  if (containsSymbol(symbol)){
    return false;
  }
  struct Symbol symbolData;
  symbolData.name = symbol;
  symbolData.address = -1;
  symbolData.isArray = false;
  symbolData.arraySize = 0;
  symbolData.initialized = false;
  symbolData.dontStore = false;

  symbol_map[symbol] = symbolData;
  symbol_list.push_back(symbol);

  return true;
}

bool SymbolTable::addArraySymbol(std::string symbol, mpz_class size){
  if (containsSymbol(symbol) || size <= 0){
    return false;
  }
  struct Symbol symbolData;
  symbolData.name = symbol;
  symbolData.address = -1;
  symbolData.isArray = true;
  symbolData.arraySize = size;
  symbolData.initialized = false;
  symbolData.dontStore = false;

  symbol_map[symbol] = symbolData;
  symbol_list.push_back(symbol);

  return true;
}

bool SymbolTable::addConstant(mpz_class constant){
  if (containsConstant(constant)){
    return false;
  }
  struct Symbol symbolData;
  std::stringstream ss;
  ss << "CONST_";
  ss << constant;
  symbolData.name = ss.str();
  symbolData.address = -1;
  symbolData.isArray = false;
  symbolData.arraySize = 0;
  symbolData.initialized = true;
  symbolData.dontStore = false;

  constants_map[constant] = symbolData;
  constants.insert(constant);

  return true;
}

bool SymbolTable::addConstants(std::set<mpz_class> constants){
  for(auto constant : constants){
    addConstant(constant);
  }
  return true;
}

std::string SymbolTable::addTempSymbol(bool dontStore){
  std::stringstream ss;
  ss << "TMP_";
  ss << lastTempSymbol;
  lastTempSymbol++;
  std::string symbol = ss.str();

  struct Symbol symbolData;
  symbolData.name = symbol;
  symbolData.address = -1;
  symbolData.isArray = false;
  symbolData.arraySize = 0;
  symbolData.initialized = true;
  symbolData.dontStore = dontStore;

  symbol_map[symbol] = symbolData;
  temp_symbols.insert(symbol);

  return symbol;
}

std::string SymbolTable::addTempSymbol(){
  return addTempSymbol(false);
}

bool SymbolTable::pushIterator(std::string symbol){
  if (iteratorOnStack(symbol) || containsSymbol(symbol)){
    return false;
  }
  struct Symbol symbolData;
  symbolData.name = symbol;
  symbolData.address = -1;
  symbolData.isArray = false;
  symbolData.arraySize = 0;
  symbolData.initialized = true;

  symbol_map[symbol] = symbolData;
  iterator_stack.push_back(symbol);

  return true;
}

std::string SymbolTable::popIterator(){
  std::string symbol = iterator_stack.back();
  iterator_stack.pop_back();
  iterator_list.insert(symbol);

  return symbol;
}

mpz_class SymbolTable::getSymbol(std::string symbol){
  if (symbol_map.count(symbol)){
    return symbol_map[symbol].address;
  }
  return -1;
}

mpz_class SymbolTable::getArrayAddress(std::string symbol){
  return symbol_map[symbol].address + 1;
}

mpz_class SymbolTable::getArraySize(std::string symbol){
  return symbol_map[symbol].arraySize;
}

mpz_class SymbolTable::getConstant(mpz_class constant){
  return constants_map[constant].address;
}

std::vector<std::string> SymbolTable::getSymbols(){
  return symbol_list;
}

std::set<std::string> SymbolTable::getIterators(){
  return iterator_list;
}

bool SymbolTable::hasDontStoreFlag(std::string symbol){
  if (symbol_map.count(symbol)){
    return symbol_map[symbol].dontStore;
  }
  return true;
}

bool SymbolTable::containsSymbol(std::string symbol){
  if(std::find(symbol_list.begin(), symbol_list.end(), symbol) != symbol_list.end()) {
    return true;
  }
  return false;
}

bool SymbolTable::containsConstant(mpz_class constant){
  return constants.count(constant) == 1;
}


bool SymbolTable::containsIterator(std::string symbol){
  return iterator_list.count(symbol) == 1;
}

bool SymbolTable::iteratorOnStack(std::string symbol){
  if(std::find(iterator_stack.begin(), iterator_stack.end(), symbol) != iterator_stack.end()) {
    return true;
  }
  return false;
}

void SymbolTable::initialize(std::string symbol){
  symbol_map[symbol].initialized = true;
}

bool SymbolTable::isInitialized(std::string symbol){
  return symbol_map[symbol].initialized;
}

bool SymbolTable::isArray(std::string symbol){
  return symbol_map[symbol].isArray;
}

void SymbolTable::printSymbolData(std::string symbol){
  struct Symbol symbolData = symbol_map[symbol];
  if (symbolData.isArray){
    std::cerr << symbolData.name << "[" << symbolData.arraySize << "] " << symbolData.address << std::endl;
  } else {
    std::cerr << symbolData.name << " " << symbolData.address << std::endl;
  }
}

/**
 Generate assembly code for generating a constant.
**/
AssemblyCode SymbolTable::constantCode(mpz_class number){
  mpz_class addr = getConstant(number);
  AssemblyCode code;
  while (number > 0){
    mpz_class remainder = number % 2;
    if (remainder == 1){
      code.pushToFront(AsmInstruction::Increase);
    }
    if (number > 1){
      code.pushToFront(AsmInstruction::ShiftLeft);
    }
    number /= 2;
  }
  code.pushToFront(AsmInstruction::Zero);
  code.pushInstruction(AsmInstruction::Store, addr);
  return code;
}

/**
 Generate assembly code to generate all constants.
**/
AssemblyCode SymbolTable::generateConstantsCode(){
  AssemblyCode code;
  for(auto constant : constants){
    AssemblyCode generated = constantCode(constant);
    code.pushCode(generated);
  }
  return code;
}
