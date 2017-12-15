#ifndef COMMAND_HH
#define COMMAND_HH

#include <gmp.h>
#include <gmpxx.h>
#include <string.h>
#include <vector>
#include <ostream>
#include "Identifier.hh"
#include "Value.hh"
#include "Expression.hh"

class IAddress {
public:
  virtual mpz_class getAddress() = 0;
  virtual std::string toStr() const = 0;
  virtual bool isUnparsedIdentifier();
  virtual bool isUnparsedValue();
};

class UnparsedIdentifier : public IAddress {
public:
  Identifier* identifier;
  mpz_class getAddress();

  UnparsedIdentifier(Identifier* id);
  std::string toStr() const;
  bool isUnparsedIdentifier();
  bool isUnparsedValue();
};

class UnparsedValue : public IAddress {
public:
  Value* value;
  mpz_class getAddress();

  UnparsedValue(Value* val);
  std::string toStr() const;
  bool isUnparsedIdentifier();
  bool isUnparsedValue();
};

class AddrVariable : public IAddress {
public:
  std::string symbol;
  virtual mpz_class getAddress();

  AddrVariable(std::string symbol);
  std::string toStr() const;
};

class AddrConstant : public IAddress {
public:
  mpz_class value;
  virtual mpz_class getAddress();

  AddrConstant(mpz_class value);
  std::string toStr() const;
};

class AddrPointer : public IAddress {
public:
  std::string symbol;
  virtual mpz_class getAddress();
};

class AddrLabel : public IAddress {
public:
  std::string label;
  virtual mpz_class getAddress();

  AddrLabel(std::string label);
  std::string toStr() const;
};

enum CommandType {
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
  JumpOdd
};

class Command {
public:
  CommandType command;
  IAddress* addr1;
  IAddress* addr2;
  IAddress* addr3;

  Command(CommandType, IAddress*, IAddress*, IAddress*);
  Command(CommandType, IAddress*, IAddress*);
  Command(CommandType, IAddress*);
  Command(Identifier*, Expression*);
  Command(CommandType cmd, Identifier*);
  Command(CommandType cmd, Value*);

  std::vector<Command*> convertToTriAddress();
};

std::ostream& operator<<(std::ostream& strm, const IAddress& a);
std::ostream& operator<<(std::ostream &strm, const Command &a) ;

#endif
