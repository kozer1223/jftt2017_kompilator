#include "CommandBlock.hh"
#include <iostream>

CommandBlock::CommandBlock(SymbolTable* symbolTable){
  nextBlock = nullptr;
  conditionalLink = nullptr;
  globalSymbolTable = symbolTable;
}

void CommandBlock::pushCommand(Command* cmd){
  commands.push_back(cmd);
}

void CommandBlock::pushCommandSet(CommandSet* cmds){
  if (cmds == nullptr)
    return;
  //std::cout << cmds << std::endl;
  //std::cout << *cmds;
  //std::cout << "THIS" << std::endl;
  //std::cout << *this;
  commands.insert(commands.end(), (cmds->commands).begin(), (cmds->commands).end());
  //std::cout << "inserted" << std::endl;
  //std::cout << *(commands[0]) << std::endl;
  //for (auto asdfg : commands){
  //  std::cout << " o co tu chodzi?? " << std::endl;
  //  if (asdfg != nullptr)
  //    std::cout << (*asdfg) << std::endl;
  //}
  //std::cout << "saffasaf" << std::endl;
}

void CommandBlock::insertCommands(CommandBlock* cmdBlocks){
  // TODO move local symbols
  commands.insert(commands.end(), cmdBlocks->commands.begin(), cmdBlocks->commands.end());
}

void CommandBlock::insertCommands(std::vector<Command*> cmds){
  commands.insert(commands.end(), cmds.begin(), cmds.end());
}

/**
Converts all commands to appropriate tri-address form.
Symbols are converted to AddrVariable,
numeric values are converted to AddrConstant,
arrays are turned to appropriate pointers (AddrPointer)
**/
void CommandBlock::convertToTriAddress() {
  std::vector<Command*> newCommands;
  for (auto command : commands){
    if (command != nullptr) {
      std::vector<Command*> replacement = command->convertToTriAddress();
      newCommands.insert(newCommands.end(), replacement.begin(), replacement.end());
      delete command;
    }
  }
  commands = newCommands;
}

std::ostream& operator<<(std::ostream &strm, const CommandBlock &a) {
  for (auto command : a.commands){
    if (command != nullptr)
      strm << (*command) << std::endl;
  }
  return strm;
}
