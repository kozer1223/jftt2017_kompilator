#ifndef COMMANDBLOCK_HH
#define COMMANDBLOCK_HH

#include <string>
#include <set>
#include <vector>
#include <memory>
#include <map>
#include <gmp.h>
#include <gmpxx.h>
#include <algorithm>
#include "Command.hh"
#include "CommandSet.hh"
#include "SymbolTable.hh"
#include "LabelManager.hh"
#include "AssemblyCode.hh"

class CommandBlock{
public:
  std::vector<Command> commands;
  std::shared_ptr<CommandBlock> nextBlock;
  CommandBlock* jumpBlock;
  // condition
  std::set<CommandBlock*> fromBlocks;
  std::set<std::string> labels;
  bool unconditionalJump = false;

  SymbolTable* globalSymbolTable;

  CommandBlock();
  CommandBlock(SymbolTable* symbolTable);
  //~CommandBlock();

  void pushCommand(Command cmd);
  void pushCommandSet(CommandSet cmds);
  void insertCommands(CommandBlock cmdBlock);
  void insertCommands(std::vector<Command> cmds);
  void convertToTriAddress();
  void splitToBlocks();
  void splitToBlocks(std::map<std::string, CommandBlock*>&);
  void optimize();
  AssemblyCode generateMultiplication(IAddress*, IAddress*, IAddress*, LabelManager*);
  AssemblyCode generateDivision(IAddress*, IAddress*, IAddress*, LabelManager*);
  AssemblyCode generateModulo(IAddress*, IAddress*, IAddress*, LabelManager*);
  AssemblyCode getAssembly(LabelManager*);
  std::set<mpz_class> getConstants();
  std::string preBlockRegisterState() const;
  std::string postBlockRegisterState() const;
};

std::ostream& operator<<(std::ostream &strm, const CommandBlock &a);

#endif
