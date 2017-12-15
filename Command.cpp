#include "Command.hh"
#include <sstream>
#include <iostream>

bool IAddress::isUnparsedIdentifier(){
  return false;
}

bool IAddress::isUnparsedValue(){
  return false;
}

UnparsedIdentifier::UnparsedIdentifier(Identifier* id){
  identifier = id;
}

mpz_class UnparsedIdentifier::getAddress(){
  return -1;
}

bool UnparsedIdentifier::isUnparsedIdentifier(){
  return true;
}

bool UnparsedIdentifier::isUnparsedValue(){
  return false;
}

UnparsedValue::UnparsedValue(Value* val){
  value = val;
}

mpz_class UnparsedValue::getAddress(){
  return -1;
}

bool UnparsedValue::isUnparsedIdentifier(){
  return false;
}

bool UnparsedValue::isUnparsedValue(){
  return true;
}

AddrVariable::AddrVariable(std::string s){
  symbol = s;
}

AddrConstant::AddrConstant(mpz_class val){
  value = val;
}

AddrLabel::AddrLabel(std::string l){
  label = l;
}

mpz_class AddrVariable::getAddress(){
  return -1;
}

mpz_class AddrConstant::getAddress(){
  return -1;
}

mpz_class AddrLabel::getAddress(){
  return -1;
}


Command::Command(CommandType cmd, IAddress* a1, IAddress* a2, IAddress* a3){
  command = cmd;
  addr1 = a1;
  addr2 = a2;
  addr3 = a3;
}

Command::Command(CommandType cmd, IAddress* a1, IAddress* a2){
  command = cmd;
  addr1 = a1;
  addr2 = a2;
  addr3 = nullptr;
}

Command::Command(CommandType cmd, IAddress* a1){
  command = cmd;
  addr1 = a1;
  addr2 = nullptr;
  addr3 = nullptr;
}

Command::Command(Identifier* lhs_id, Expression* expr){
  addr1 = new UnparsedIdentifier(lhs_id);
  addr2 = nullptr;
  addr3 = nullptr;
  if (expr->lhs != nullptr)
    addr2 = new UnparsedValue(expr->lhs);
  expr->lhs = nullptr;
  if (expr->rhs != nullptr)
    addr3 = new UnparsedValue(expr->rhs);
  expr->rhs = nullptr;
  switch(expr->operation){
    case ExprOp::Nop: command = CommandType::Assign; break;
    case ExprOp::Add: command = CommandType::AssignAdd; break;
    case ExprOp::Sub: command = CommandType::AssignSub; break;
    case ExprOp::Mul: command = CommandType::AssignMul; break;
    case ExprOp::Div: command = CommandType::AssignDiv; break;
    case ExprOp::Mod: command = CommandType::AssignMod; break;
  }
  delete expr;
}

Command::Command(CommandType cmd, Identifier* id){
  addr1 = new UnparsedIdentifier(id);
  addr2 = nullptr;
  addr3 = nullptr;
  command = cmd;
}

Command::Command(CommandType cmd, Value* val){
  addr1 = new UnparsedValue(val);
  addr2 = nullptr;
  addr3 = nullptr;
  command = cmd;
}

std::vector<Command*> Command::convertToTriAddress(){
  std::vector<Command*> newCommands;
  IAddress* new_addr1 = addr1;
  IAddress* new_addr2 = addr2;
  IAddress* new_addr3 = addr3;

  //std::cout << "I am " << *this << std::endl;
  if (addr1 != nullptr){
    //std::cout << "check if fix addr1" << std::endl;
    if (addr1->isUnparsedIdentifier()){
      // TODO: check for arrays
      new_addr1 = new AddrVariable( ((UnparsedIdentifier*)addr1)->identifier->symbol );
      delete addr1;
    } else if (addr1->isUnparsedValue()){
      Value* val = ((UnparsedValue*)addr1)->value;
      if (val->type == ValueType::VIdentifier){
        // TODO: check for arrays
        new_addr1 = new AddrVariable( val->value.identifier->symbol );
      } else {
        new_addr1 = new AddrConstant( *(val->value.number) );
      }
      delete addr1;
    }
  }

  if (addr2 != nullptr){
    //std::cout << "check if fix addr2" << std::endl;
    if (addr2->isUnparsedIdentifier()){
      // TODO: check for arrays
      new_addr2 = new AddrVariable( ((UnparsedIdentifier*)addr2)->identifier->symbol );
      delete addr2;
    } else if (addr2->isUnparsedValue()){
      Value* val = ((UnparsedValue*)addr2)->value;
      if (val->type == ValueType::VIdentifier){
        // TODO: check for arrays
        new_addr2 = new AddrVariable( val->value.identifier->symbol );
      } else {
        new_addr2 = new AddrConstant( *(val->value.number) );
      }
      delete addr2;
    }
  }

  if (addr3 != nullptr){
    //std::cout << "check if fix addr3" << std::endl;
    if (addr3->isUnparsedIdentifier()){
      // TODO: check for arrays
      new_addr3 = new AddrVariable( ((UnparsedIdentifier*)addr3)->identifier->symbol );
      delete addr3;
    } else if (addr3->isUnparsedValue()){
      Value* val = ((UnparsedValue*)addr3)->value;
      if (val->type == ValueType::VIdentifier){
        // TODO: check for arrays
        new_addr3 = new AddrVariable( val->value.identifier->symbol );
      } else {
        new_addr3 = new AddrConstant( *(val->value.number) );
      }
      delete addr3;
    }
  }

  newCommands.push_back( new Command(command, new_addr1, new_addr2, new_addr3) );
  return newCommands;
}

std::string UnparsedIdentifier::toStr() const {
  std::stringstream stream;
  stream << *identifier;
  return stream.str();
}

std::string UnparsedValue::toStr() const {
  std::stringstream stream;
  stream << *value;
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
  }
  return strm;
}
