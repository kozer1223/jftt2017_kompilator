#ifndef COMMAND_HH
#define COMMAND_HH

#include <gmp.h>
#include <gmpxx.h>
#include <string.h>
#include <vector>
#include <set>
#include <ostream>
#include <memory>
#include "Identifier.hh"
#include "Value.hh"
#include "Expression.hh"
#include "SymbolTable.hh"

namespace Register{
  const std::string UNDEFINED_REGISTER_STATE = "-";
}

class IAddress {
public:
  virtual mpz_class getAddress(SymbolTable*) = 0;
  virtual std::string toStr() const = 0;
  virtual bool isUnparsedIdentifier();
  virtual bool isUnparsedValue();
  virtual bool isConstant();
  virtual bool isPointer();
  virtual mpz_class constValue();

  virtual ~IAddress(){};
};

class UnparsedIdentifier : public IAddress {
public:
  Identifier identifier;
  mpz_class getAddress(SymbolTable*);

  UnparsedIdentifier(Identifier id): identifier(id) {}
  std::string toStr() const;
  bool isUnparsedIdentifier();
  bool isUnparsedValue();

  ~UnparsedIdentifier();
};

class UnparsedValue : public IAddress {
public:
  Value value;
  mpz_class getAddress(SymbolTable*);

  UnparsedValue(Value val): value(val) {}
  std::string toStr() const;
  bool isUnparsedIdentifier();
  bool isUnparsedValue();

  ~UnparsedValue();
};

class AddrVariable : public IAddress {
public:
  std::string symbol;
  mpz_class getAddress(SymbolTable*);

  AddrVariable(std::string symbol);
  ~AddrVariable();
  std::string toStr() const;
};

class AddrConstant : public IAddress {
public:
  mpz_class value;
  mpz_class getAddress(SymbolTable*);

  AddrConstant(mpz_class value);
  ~AddrConstant();
  std::string toStr() const;
  bool isConstant();
  mpz_class constValue();
};

class AddrPointer : public IAddress {
public:
  std::string symbol;
  mpz_class getAddress(SymbolTable*);

  AddrPointer(std::string symbol);
  ~AddrPointer();
  bool isPointer();
  std::string toStr() const;
};

class AddrLabel : public IAddress {
public:
  std::string label;
  mpz_class getAddress(SymbolTable*);

  AddrLabel(std::string label);
  ~AddrLabel();
  std::string toStr() const;
};

enum class CommandType {
  Label,
  Halt,
  Read,
  Write,
  Assign,
  AssignAdd,
  AssignSub,
  AssignMul,
  AssignDiv,
  AssignMod,
  Jump,
  JumpZero,
  JumpOdd,
  Increase,
  Decrease,
  ShiftLeft,
  ShiftRight
};

class Command {
public:
  CommandType command;
  std::shared_ptr<IAddress> addr1;
  std::shared_ptr<IAddress> addr2;
  std::shared_ptr<IAddress> addr3;

  Command(const Command& cmd);
  Command(CommandType, std::shared_ptr<IAddress>, std::shared_ptr<IAddress>, std::shared_ptr<IAddress>);
  Command(CommandType, std::shared_ptr<IAddress>, std::shared_ptr<IAddress>);
  Command(CommandType, std::shared_ptr<IAddress>);
  Command(CommandType, IAddress*, IAddress*, IAddress*);
  Command(CommandType, IAddress*, IAddress*);
  Command(CommandType, IAddress*);
  Command(Identifier, Expression);
  Command(CommandType cmd, Identifier);
  Command(CommandType cmd, Value);
  ~Command();

  std::vector<Command> convertToTriAddress(SymbolTable* st);
  std::set<mpz_class> getConstants();
  std::string postCommandRegisterState() const;
  std::string preCommandRegisterState() const;
};

std::ostream& operator<<(std::ostream& strm, const IAddress& a);
std::ostream& operator<<(std::ostream &strm, const Command &a) ;

#endif
