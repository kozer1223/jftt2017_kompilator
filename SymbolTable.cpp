#include "SymbolTable.hh"
#include <iostream>
#include <sstream>

SymbolTable::SymbolTable(){
  lastFreeAddress = 10; // reserve extra space
  lastTempSymbol = 1;
}

void SymbolTable::allocateSymbols(){
  lastFreeAddress = 10;
  for (auto symbol : symbol_list) {
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

bool SymbolTable::addSymbol(std::string symbol){
  if (containsSymbol(symbol)){
    return false;
  }
  struct Symbol symbolData;
  symbolData.name = symbol;
  symbolData.address = -1;
  symbolData.isArray = false;
  symbolData.arraySize = 0;

  symbol_map[symbol] = symbolData;
  symbol_list.push_back(symbol);

  return true;
}

std::string SymbolTable::addTempSymbol(){
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

  symbol_map[symbol] = symbolData;
  temp_symbols.insert(symbol);

  return symbol;
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
  return symbol_map[symbol].address;
}

std::vector<std::string> SymbolTable::getSymbols(){
  return symbol_list;
}

std::set<std::string> SymbolTable::getIterators(){
  return iterator_list;
}

bool SymbolTable::containsSymbol(std::string symbol){
  if(std::find(symbol_list.begin(), symbol_list.end(), symbol) != symbol_list.end()) {
    return true;
  }
  return false;
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

void SymbolTable::printSymbolData(std::string symbol){
  struct Symbol symbolData = symbol_map[symbol];
  std::cout << symbolData.name << " " << symbolData.address << std::endl;
}
