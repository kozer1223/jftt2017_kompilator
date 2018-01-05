#include "Command.hh"
#include <sstream>
#include <iostream>

bool IAddress::isUnparsedIdentifier(){
  return false;
}

bool IAddress::isUnparsedValue(){
  return false;
}

bool IAddress::isConstant(){
  return false;
}

bool IAddress::isPointer(){
  return false;
}

mpz_class IAddress::constValue(){
  return -1;
}

mpz_class UnparsedIdentifier::getAddress(SymbolTable* symTab){
  return -1;
}

UnparsedIdentifier::~UnparsedIdentifier(){}

bool UnparsedIdentifier::isUnparsedIdentifier(){
  return true;
}

bool UnparsedIdentifier::isUnparsedValue(){
  return false;
}

mpz_class UnparsedValue::getAddress(SymbolTable* symTab){
  return -1;
}

UnparsedValue::~UnparsedValue(){}

bool UnparsedValue::isUnparsedIdentifier(){
  return false;
}

bool UnparsedValue::isUnparsedValue(){
  return true;
}

AddrVariable::AddrVariable(std::string s){
  symbol = s;
}

AddrVariable::~AddrVariable(){}

AddrConstant::AddrConstant(mpz_class val){
  value = val;
}

AddrConstant::~AddrConstant(){};

bool AddrConstant::isConstant(){
  return true;
}

mpz_class AddrConstant::constValue(){
  return value;
}

AddrPointer::AddrPointer(std::string s){
  symbol = s;
}

AddrPointer::~AddrPointer(){}

bool AddrPointer::isPointer(){
  return true;
}

AddrLabel::AddrLabel(std::string l){
  label = l;
}

AddrLabel::~AddrLabel(){}

mpz_class AddrVariable::getAddress(SymbolTable* symTab){
  return symTab->getSymbol(symbol);
}

mpz_class AddrConstant::getAddress(SymbolTable* symTab){
  return symTab->getConstant(value);
}

mpz_class AddrPointer::getAddress(SymbolTable* symTab){
  return symTab->getSymbol(symbol);
}

mpz_class AddrLabel::getAddress(SymbolTable* symTab){
  return -1;
}

Command::Command(const Command& cmd){
  command = cmd.command;
  addr1 = cmd.addr1;
  addr2 = cmd.addr2;
  addr3 = cmd.addr3;
}

Command::Command(CommandType cmd, std::shared_ptr<IAddress> a1, std::shared_ptr<IAddress> a2, std::shared_ptr<IAddress> a3){
  command = cmd;
  addr1 = a1;//std::move(a1);
  addr2 = a2;//std::move(a2);
  addr3 = a3;//std::move(a3);
}

Command::Command(CommandType cmd, std::shared_ptr<IAddress> a1, std::shared_ptr<IAddress> a2){
  command = cmd;
  addr1 = a1;//std::move(a1);
  addr2 = a2;//std::move(a2);
  addr3 = std::shared_ptr<IAddress>(nullptr);
}

Command::Command(CommandType cmd, std::shared_ptr<IAddress> a1){
  command = cmd;
  addr1 = a1;//std::move(a1);
  addr2 = std::shared_ptr<IAddress>(nullptr);
  addr3 = std::shared_ptr<IAddress>(nullptr);
}

Command::Command(CommandType cmd, IAddress* a1, IAddress* a2, IAddress* a3){
  command = cmd;
  addr1 = std::shared_ptr<IAddress>(a1);
  addr2 = std::shared_ptr<IAddress>(a2);
  addr3 = std::shared_ptr<IAddress>(a3);
}

Command::Command(CommandType cmd, IAddress* a1, IAddress* a2){
  command = cmd;
  addr1 = std::shared_ptr<IAddress>(a1);
  addr2 = std::shared_ptr<IAddress>(a2);
  addr3 = std::shared_ptr<IAddress>(nullptr);
}

Command::Command(CommandType cmd, IAddress* a1){
  command = cmd;
  addr1 = std::shared_ptr<IAddress>(a1);
  addr2 = std::shared_ptr<IAddress>(nullptr);
  addr3 = std::shared_ptr<IAddress>(nullptr);
}

Command::Command(Identifier lhs_id, Expression expr){
  addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(lhs_id));
  addr2 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>(expr.lhs));
  addr3 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>(expr.rhs));
  switch(expr.operation){
    case ExprOp::Nop: command = CommandType::Assign; break;
    case ExprOp::Add: command = CommandType::AssignAdd; break;
    case ExprOp::Sub: command = CommandType::AssignSub; break;
    case ExprOp::Mul: command = CommandType::AssignMul; break;
    case ExprOp::Div: command = CommandType::AssignDiv; break;
    case ExprOp::Mod: command = CommandType::AssignMod; break;
  }
}

Command::Command(CommandType cmd, Identifier id){
  addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedIdentifier>(id));
  addr2 = std::shared_ptr<IAddress>(nullptr);
  addr3 = std::shared_ptr<IAddress>(nullptr);
  command = cmd;
}

Command::Command(CommandType cmd, Value val){
  addr1 = std::shared_ptr<IAddress>(std::make_shared<UnparsedValue>(val));
  addr2 = std::shared_ptr<IAddress>(nullptr);
  addr3 = std::shared_ptr<IAddress>(nullptr);
  command = cmd;
}

Command::~Command(){
}

std::shared_ptr<IAddress> parseAddress(std::shared_ptr<IAddress> addr, std::vector<Command> & commands, SymbolTable* st){
  if (addr != nullptr){
    if (addr->isUnparsedIdentifier()){
      // TODO: check for arrays
      Identifier identifier = ((UnparsedIdentifier*)addr.get())->identifier;
      if (identifier.isArray){
        // arrays
        std::string pointer = st->addTempSymbol();
        auto pointerAddr = std::make_shared<AddrVariable>( pointer );
        auto arrAddr = std::make_shared<AddrConstant>( st->getArrayAddress(identifier.symbol) );
        std::shared_ptr<IAddress> offsetAddr;
        if (identifier.indexType == IndexType::Variable){
          offsetAddr = std::make_shared<AddrVariable>( identifier.symbolIndex );
        } else if (identifier.indexType == IndexType::Number){
          offsetAddr = std::make_shared<AddrConstant>( identifier.numberIndex );
        }
        Command initPointer = Command(CommandType::AssignAdd, pointerAddr, arrAddr, offsetAddr);
        commands.push_back(initPointer);
        return std::make_shared<AddrPointer>( pointer );
      } else {
        return std::make_shared<AddrVariable>( identifier.symbol );
      }
    } else if (addr->isUnparsedValue()){
      Value val = ((UnparsedValue*)addr.get())->value;
      if (val.type == ValueType::VIdentifier){
        // TODO: check for arrays
        Identifier identifier = val.identifier;
        if (identifier.isArray){
          // arrays
          std::string pointer = st->addTempSymbol();
          auto pointerAddr = std::make_shared<AddrVariable>( pointer );
          auto arrAddr = std::make_shared<AddrConstant>( st->getArrayAddress(identifier.symbol) );
          std::shared_ptr<IAddress> offsetAddr;
          if (identifier.indexType == IndexType::Variable){
            offsetAddr = std::make_shared<AddrVariable>( identifier.symbolIndex );
          } else if (identifier.indexType == IndexType::Number){
            offsetAddr = std::make_shared<AddrConstant>( identifier.numberIndex );
          }
          Command initPointer = Command(CommandType::AssignAdd, pointerAddr, arrAddr, offsetAddr);
          commands.push_back(initPointer);
          return std::make_shared<AddrPointer>( pointer );
        } else {
          return std::make_shared<AddrVariable>( identifier.symbol );
        }
      } else {
        return std::make_shared<AddrConstant>( val.number );
      }
    }
  }
  return addr;
}

std::vector<Command> Command::convertToTriAddress(SymbolTable* st){
  std::vector<Command> newCommands;
  auto new_addr1 = addr1;
  auto new_addr2 = addr2;
  auto new_addr3 = addr3;

  new_addr1 = parseAddress(addr1, newCommands, st);
  new_addr2 = parseAddress(addr2, newCommands, st);
  new_addr3 = parseAddress(addr3, newCommands, st);

  Command replacementCmd = Command(command, new_addr1, new_addr2, new_addr3);
  newCommands.push_back(replacementCmd);
  return newCommands;
}

std::set<mpz_class> Command::getConstants(){
  std::set<mpz_class> constants;
  if (addr1 != nullptr){
    if (addr1->isConstant()){
      constants.insert(addr1->constValue());
    }
  }
  if (addr2 != nullptr){
    if (addr2->isConstant()){
      constants.insert(addr2->constValue());
    }
  }
  if (addr3 != nullptr){
    if (addr3->isConstant()){
      constants.insert(addr3->constValue());
    }
  }
  return constants;
}

std::string Command::postCommandRegisterState() const{
  switch(command){
    case CommandType::Read: return addr1->toStr();
    case CommandType::Write: return addr1->toStr();
    case CommandType::Assign: return addr1->toStr();
    case CommandType::AssignAdd: return addr1->toStr();
    case CommandType::AssignSub: return addr1->toStr();
    case CommandType::AssignMul: return Register::UNDEFINED_REGISTER_STATE;
    case CommandType::AssignDiv: return Register::UNDEFINED_REGISTER_STATE;
    case CommandType::AssignMod: return Register::UNDEFINED_REGISTER_STATE;
    case CommandType::Increase: return addr1->toStr();
    case CommandType::Decrease: return addr1->toStr();
    case CommandType::ShiftLeft: return addr1->toStr();
    case CommandType::ShiftRight: return addr1->toStr();
    case CommandType::JumpZero: return addr1->toStr();
    case CommandType::JumpOdd: return addr1->toStr();
    default: return Register::UNDEFINED_REGISTER_STATE;
  }
}

std::string Command::preCommandRegisterState() const{
  switch(command){
    case CommandType::Write: return addr1->toStr();
    case CommandType::Assign: return addr2->toStr();
    case CommandType::AssignAdd: return addr2->toStr();
    case CommandType::AssignSub: return addr2->toStr();
    case CommandType::AssignMul: return addr2->toStr();
    case CommandType::AssignDiv: return addr2->toStr();
    case CommandType::AssignMod: return addr2->toStr();
    case CommandType::Increase: return addr1->toStr();
    case CommandType::Decrease: return addr1->toStr();
    case CommandType::ShiftLeft: return addr1->toStr();
    case CommandType::ShiftRight: return addr1->toStr();
    case CommandType::JumpZero: return addr1->toStr();
    case CommandType::JumpOdd: return addr1->toStr();
    default: return Register::UNDEFINED_REGISTER_STATE;
  }
}

std::string UnparsedIdentifier::toStr() const {
  std::stringstream stream;
  stream << identifier;
  return stream.str();
}

std::string UnparsedValue::toStr() const {
  std::stringstream stream;
  stream << value;
  return stream.str();
}

std::string AddrVariable::toStr() const {
  return symbol;
}

std::string AddrConstant::toStr() const {
  std::stringstream stream;
  stream << "const(" << value << ")";
  return stream.str();
}

std::string AddrPointer::toStr() const {
  std::stringstream stream;
  stream << "*" << symbol;
  return stream.str();
}

std::string AddrLabel::toStr() const {
  return label;
}

std::ostream& operator<<(std::ostream& strm, const IAddress& a){
   return strm << a.toStr();
}

std::ostream& operator<<(std::ostream &strm, const Command &a) {
  switch (a.command){
    case CommandType::Label : return strm << "label " << *(a.addr1);
    case CommandType::Halt : return strm << "halt";
    case CommandType::Read : return strm << "read " << *(a.addr1);
    case CommandType::Write : return strm << "write " << *(a.addr1);
    case CommandType::Assign : return strm << *(a.addr1) << " := " << *(a.addr2);
    case CommandType::AssignAdd : return strm << *(a.addr1) << " := " << *(a.addr2) << " + " << *(a.addr3);
    case CommandType::AssignSub : return strm << *(a.addr1) << " := " << *(a.addr2) << " - " << *(a.addr3);
    case CommandType::AssignMul : return strm << *(a.addr1) << " := " << *(a.addr2) << " * " << *(a.addr3);
    case CommandType::AssignDiv : return strm << *(a.addr1) << " := " << *(a.addr2) << " / " << *(a.addr3);
    case CommandType::AssignMod : return strm << *(a.addr1) << " := " << *(a.addr2) << " % " << *(a.addr3);
    case CommandType::Jump : return strm << "goto " << *(a.addr1);
    case CommandType::JumpZero : return strm << "if " << *(a.addr1) << " = 0 goto " << *(a.addr2);
    case CommandType::JumpOdd : return strm << "if " << *(a.addr1) << " odd goto " << *(a.addr2);
    case CommandType::Increase : return strm << *(a.addr1) << "++";
    case CommandType::Decrease : return strm << *(a.addr1) << "--";
    case CommandType::ShiftLeft : return strm << *(a.addr1) << " *= 2";
    case CommandType::ShiftRight : return strm << *(a.addr1) << " /= 2";
  }
  return strm;
}
