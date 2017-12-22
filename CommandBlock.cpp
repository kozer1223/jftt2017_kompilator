#include "CommandBlock.hh"
#include <iostream>

CommandBlock::CommandBlock(){
  nextBlock = nullptr;
  jumpBlock = nullptr;
  globalSymbolTable = nullptr;
}

CommandBlock::CommandBlock(SymbolTable* symbolTable){
  nextBlock = nullptr;
  jumpBlock = nullptr;
  globalSymbolTable = symbolTable;
}

void CommandBlock::pushCommand(Command cmd){
  commands.push_back(cmd);
}

void CommandBlock::pushCommandSet(CommandSet cmds){
  //std::cout << "INSERTING" << std::endl;
  //std::cout << cmds << std::endl;
  //std::cout << "THIS" << std::endl;
  //std::cout << *this;
  commands.insert(commands.end(), (cmds.commands).begin(), (cmds.commands).end());
  //std::cout << "inserted" << std::endl;
  //std::cout << *(commands[0]) << std::endl;
  //for (auto asdfg : commands){
  //  std::cout << " o co tu chodzi?? " << std::endl;
  //    std::cout << asdfg << std::endl;
  //}
  //std::cout << "saffasaf" << std::endl;
}

void CommandBlock::insertCommands(CommandBlock cmdBlocks){
  // TODO move local symbols
  commands.insert(commands.end(), cmdBlocks.commands.begin(), cmdBlocks.commands.end());
}

void CommandBlock::insertCommands(std::vector<Command> cmds){
  commands.insert(commands.end(), cmds.begin(), cmds.end());
}

/**
Converts all commands to appropriate tri-address form.
Symbols are converted to AddrVariable,
numeric values are converted to AddrConstant,
arrays are turned to appropriate pointers (AddrPointer)
**/
void CommandBlock::convertToTriAddress() {
  std::vector<Command> newCommands;
  for (auto & command : commands){
    std::vector<Command> replacement = command.convertToTriAddress();
    newCommands.insert(newCommands.end(), replacement.begin(), replacement.end());
  }
  commands = newCommands;
}

void CommandBlock::splitToBlocks(){
  std::map<std::string, CommandBlock*> labelMap;
  splitToBlocks(labelMap);
  //std::cout << "aaa" << std::endl;
	CommandBlock* p = this;
	while(p != nullptr){
    /*
      std::cout << "BLOCK" << std::endl;
      for(std::string lbl : p->labels){
        std::cout << lbl << ": ";
      }
      std::cout << std::endl;
      std::cout << *p << std::endl;
      std::cout << p->commands.size() << std::endl;
      std::cout << "fxng lables" << std::endl;*/

    if (!p->commands.empty()){
      Command lastCmd = p->commands.back();
      //std::cout << "last command" << std::endl;
      //std::cout << lastCmd << std::endl;
      if( lastCmd.command == CommandType::Jump ){
        std::string lbl = ((AddrLabel*)lastCmd.addr1.get())->label;
        p->jumpBlock = labelMap[lbl];
        labelMap[lbl]->fromBlocks.insert(p);
      } else if(lastCmd.command == CommandType::JumpZero ||
                lastCmd.command == CommandType::JumpOdd){
        std::string lbl = ((AddrLabel*)lastCmd.addr2.get())->label;
        p->jumpBlock = labelMap[lbl];
        labelMap[lbl]->fromBlocks.insert(p);
      }
    }
		p = p->nextBlock.get();
	}
}

void CommandBlock::splitToBlocks(std::map<std::string, CommandBlock*>& labelMap) {
  auto iter = commands.begin();
  bool collectingLabels = true;

  //std::cout << "splitting1" << std::endl;
  while (iter != commands.end()){
    if (collectingLabels){
      if ((*iter).command == CommandType::Label){
        std::string lbl = ((AddrLabel*)(*iter).addr1.get())->label;
        labels.insert( lbl );
        labelMap[lbl] = this;
        iter = commands.erase(iter);
        continue;
      } else {
        collectingLabels = false;
      }
    }

    if ((*iter).command == CommandType::Label){
      nextBlock = std::make_shared<CommandBlock>(CommandBlock(globalSymbolTable));
      nextBlock->commands.insert(nextBlock->commands.begin(), iter, commands.end());
      iter = commands.erase(iter, commands.end());
      /*std::cout << "NEW BLOCK" << std::endl;
      std::cout << *nextBlock << std::endl;*/
      nextBlock->splitToBlocks(labelMap);
      continue;
    } else if( (*iter).command == CommandType::Jump  ||
        (*iter).command == CommandType::JumpZero  ||
        (*iter).command == CommandType::JumpOdd) {
        iter++;
        nextBlock = std::make_shared<CommandBlock>(CommandBlock(globalSymbolTable));
        nextBlock->commands.insert(nextBlock->commands.begin(), iter, commands.end());
        iter = commands.erase(iter, commands.end());
        /*std::cout << "NEW BLOCK" << std::endl;
        std::cout << *nextBlock << std::endl;*/
        nextBlock->splitToBlocks(labelMap);
        continue;
    }
    iter++;
  }
}

AssemblyCode CommandBlock::getAssembly(){
  AssemblyCode code;
  for (auto label : labels){
    code.addBlockLabel(label);
  }
  auto iter = commands.begin();
  std::string registerState = Register::UNDEFINED_REGISTER_STATE;
  mpz_class registerAddress = -1;
  while(iter != commands.end()){
    Command & cmd = (*iter);
    CommandType type = cmd.command;
    if (type == CommandType::Halt){
      code.pushInstruction(AsmInstruction::Halt);
    } else if (type == CommandType::Read){
      code.pushInstruction(AsmInstruction::Get);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::Write){
      // load only when necessary
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Put);
    } else if (type == CommandType::Assign){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignAdd){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Add, cmd.addr3.get()->getAddress(globalSymbolTable));
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignSub){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Sub, cmd.addr3.get()->getAddress(globalSymbolTable));
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::Jump){
      std::string lbl = ((AddrLabel*)(*iter).addr1.get())->label;
      code.pushInstruction(AsmInstruction::Jump, lbl);
    } else if (type == CommandType::JumpZero){
      std::string lbl = ((AddrLabel*)(*iter).addr2.get())->label;
      code.pushInstruction(AsmInstruction::JumpZero, lbl);
    } else if (type == CommandType::JumpOdd){
      std::string lbl = ((AddrLabel*)(*iter).addr2.get())->label;
      code.pushInstruction(AsmInstruction::JumpOdd, lbl);
    } else if (type == CommandType::Increase){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Increase);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::Decrease){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Decrease);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::ShiftLeft){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::ShiftLeft);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::ShiftRight){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(AsmInstruction::Load, cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::ShiftRight);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    }
    // add store instruction only when necessary (register needs to change)
    if (iter == commands.end()){
      //std::cerr << cmd << std::endl;
      if (registerAddress != -1){
        code.pushInstruction(AsmInstruction::Store, registerAddress);
      }
    } else if (cmd.postCommandRegisterState() != (*(std::next(iter))).preCommandRegisterState()){
      //std::cerr << cmd << std::endl;
      //std::cerr << cmd.postCommandRegisterState() << " -> " << (*(std::next(iter))).preCommandRegisterState() << std::endl;
      if (registerAddress != -1){
        code.pushInstruction(AsmInstruction::Store, registerAddress);
      }
    } else if (cmd.postCommandRegisterState() != (*(std::next(iter))).postCommandRegisterState()){
      //std::cerr << cmd << std::endl;
      //std::cerr << cmd.postCommandRegisterState() << " -> " << (*(std::next(iter))).postCommandRegisterState() << std::endl;
      if (registerAddress != -1){
        code.pushInstruction(AsmInstruction::Store, registerAddress);
      }
    } else {
      //std::cerr << cmd << std::endl;
      //std::cerr << cmd.postCommandRegisterState() << " -> " << (*(std::next(iter))).preCommandRegisterState() << std::endl;
    }
    if (type != CommandType::Write){
      registerState = cmd.postCommandRegisterState();
    }
    iter++;
  }
  return code;
}

std::set<mpz_class> CommandBlock::getConstants(){
  std::set<mpz_class> constants;
  for (auto & command : commands){
    std::set<mpz_class> cmd_consts = command.getConstants();
    //std::set<mpz_class> temp = constants;
    //std::set_union(temp.begin(), temp.end(), cmd_consts.begin(), cmd_consts.end(), constants.begin());
    constants.insert(cmd_consts.begin(), cmd_consts.end());
  }
  for (auto num : constants) {
    std::cerr << num << ", ";
  }
  std::cerr << std::endl;
  return constants;
}

std::ostream& operator<<(std::ostream &strm, const CommandBlock &a) {
  for (auto & command : a.commands){
    strm << command << std::endl;
  }
  return strm;
}
