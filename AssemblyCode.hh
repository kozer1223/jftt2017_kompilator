#ifndef ASSEMBLYCODE_HH
#define ASSEMBLYCODE_HH

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <gmp.h>
#include <gmpxx.h>

enum class AsmInstruction {
  Get,
  Put,
  Load,
  LoadIndirect,
  Store,
  StoreIndirect,
  Add,
  AddIndirect,
  Sub,
  SubIndirect,
  ShiftRight,
  ShiftLeft,
  Increase,
  Decrease,
  Zero,
  Jump,
  JumpZero,
  JumpOdd,
  Halt
};

inline std::string instructionCode(AsmInstruction in){
  switch(in){
    case AsmInstruction::Get: return "GET"; break;
    case AsmInstruction::Put: return "PUT"; break;
    case AsmInstruction::Load: return "LOAD"; break;
    case AsmInstruction::LoadIndirect: return "LOADI"; break;
    case AsmInstruction::Store: return "STORE"; break;
    case AsmInstruction::StoreIndirect: return "STOREI"; break;
    case AsmInstruction::Add: return "ADD"; break;
    case AsmInstruction::AddIndirect: return "ADDI"; break;
    case AsmInstruction::Sub: return "SUB"; break;
    case AsmInstruction::SubIndirect: return "SUBI"; break;
    case AsmInstruction::ShiftLeft: return "SHL"; break;
    case AsmInstruction::ShiftRight: return "SHR"; break;
    case AsmInstruction::Increase: return "INC"; break;
    case AsmInstruction::Decrease: return "DEC"; break;
    case AsmInstruction::Zero: return "ZERO"; break;
    case AsmInstruction::Jump: return "JUMP"; break;
    case AsmInstruction::JumpZero: return "JZERO"; break;
    case AsmInstruction::JumpOdd: return "JODD"; break;
    case AsmInstruction::Halt: return "HALT"; break;
  }
  return "";
}

class AssemblyCode {
public:
  std::vector<std::string> instructionSet;
  std::map<std::string, int> labelLines;

  void pushToFront(AsmInstruction);
  void pushToFront(AsmInstruction, mpz_class);
  void pushToFront(AsmInstruction, std::string);
  void pushInstruction(AsmInstruction);
  void pushInstruction(AsmInstruction, mpz_class);
  void pushInstruction(AsmInstruction, std::string);
  void addBlockLabel(std::string);
  void pushLabel(std::string);
  void pushCode(AssemblyCode&);
  std::string toString();

};

inline void AssemblyCode::pushToFront(AsmInstruction in) {
  std::stringstream ss;
  ss << instructionCode(in);
  instructionSet.insert(instructionSet.begin(), ss.str());
  for (auto lbl : labelLines){
    labelLines[lbl.first]++;
  }
}

inline void AssemblyCode::pushToFront(AsmInstruction in, mpz_class n) {
  std::stringstream ss;
  ss << instructionCode(in);
  ss << " ";
  ss << n;
  instructionSet.insert(instructionSet.begin(), ss.str());
  for (auto lbl : labelLines){
    labelLines[lbl.first]++;
  }
}

inline void AssemblyCode::pushToFront(AsmInstruction in, std::string label) {
  std::stringstream ss;
  ss << instructionCode(in);
  ss << " *";
  ss << label;
  instructionSet.insert(instructionSet.begin(), ss.str());
  for (auto lbl : labelLines){
    labelLines[lbl.first]++;
  }
}

inline void AssemblyCode::pushInstruction(AsmInstruction in) {
  std::stringstream ss;
  ss << instructionCode(in);
  instructionSet.push_back(ss.str());
}

inline void AssemblyCode::pushInstruction(AsmInstruction in, mpz_class n) {
  std::stringstream ss;
  ss << instructionCode(in);
  ss << " ";
  ss << n;
  instructionSet.push_back(ss.str());
}

inline void AssemblyCode::pushInstruction(AsmInstruction in, std::string label) {
  std::stringstream ss;
  ss << instructionCode(in);
  ss << " *";
  ss << label;
  instructionSet.push_back(ss.str());
}

inline void AssemblyCode::addBlockLabel(std::string label){
  // add to beginning
  labelLines[label] = 0;
}

inline void AssemblyCode::pushLabel(std::string label){
  // add to the end
  labelLines[label] = instructionSet.size();
}

inline void AssemblyCode::pushCode(AssemblyCode& ac){
  int offset = instructionSet.size();
  instructionSet.insert(instructionSet.end(), ac.instructionSet.begin(), ac.instructionSet.end());
  for (auto lbl : ac.labelLines){
    labelLines[lbl.first] = lbl.second + offset;
  }
}

inline std::string AssemblyCode::toString(){
  std::stringstream ss;
  for (auto ins : instructionSet){
    // replace labels
    std::string indexed_ins = ins;
    std::size_t found = indexed_ins.find("*");
    if (found != std::string::npos){
      std::string label = indexed_ins.substr(found+1);
      std::stringstream cnv;
      cnv << labelLines[label];
      indexed_ins.replace(found, std::string::npos, cnv.str());
    }
    ss << indexed_ins << std::endl;
  }
  return ss.str();
}

#endif
