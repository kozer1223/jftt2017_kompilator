#ifndef COMMANDBLOCK_HH
#define COMMANDBLOCK_HH

#include <set>
#include <vector>
#include "Command.hh"
#include "CommandSet.hh"
#include "SymbolTable.hh"

class CommandBlock{
public:
  std::vector<Command*> commands;
  CommandBlock* nextBlock;
  CommandBlock* conditionalLink;
  // condition
  std::set<CommandBlock*> fromBlocks;

  SymbolTable* globalSymbolTable;

  CommandBlock(SymbolTable* symbolTable);
  //~CommandBlock();

  void pushCommand(Command* cmd);
  void pushCommandSet(CommandSet* cmds);
  void insertCommands(CommandBlock* cmdBlock);
  void insertCommands(std::vector<Command*> cmds);
  void convertToTriAddress();
};

std::ostream& operator<<(std::ostream &strm, const CommandBlock &a);

#endif
