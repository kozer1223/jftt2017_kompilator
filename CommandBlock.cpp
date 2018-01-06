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
    std::vector<Command> replacement = command.convertToTriAddress(globalSymbolTable);
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
      nextBlock->fromBlocks.insert(this);
      continue;
    } else if( (*iter).command == CommandType::Jump  ||
        (*iter).command == CommandType::JumpZero  ||
        (*iter).command == CommandType::JumpOdd) {
        auto jumpCommand = (*iter).command;
        iter++;
        nextBlock = std::make_shared<CommandBlock>(CommandBlock(globalSymbolTable));
        nextBlock->commands.insert(nextBlock->commands.begin(), iter, commands.end());
        iter = commands.erase(iter, commands.end());
        /*std::cout << "NEW BLOCK" << std::endl;
        std::cout << *nextBlock << std::endl;*/
        nextBlock->splitToBlocks(labelMap);
        if (jumpCommand != CommandType::Jump){
          nextBlock->fromBlocks.insert(this);
        } else {
          unconditionalJump = true;
        }
        continue;
    }
    iter++;
  }
}

AsmInstruction normalizeInstruction(AsmInstruction in, IAddress* addr){
  if (addr->isPointer()){
    switch(in){
      case AsmInstruction::Load: return AsmInstruction::LoadIndirect;
      case AsmInstruction::Store: return AsmInstruction::StoreIndirect;
      case AsmInstruction::Add: return AsmInstruction::AddIndirect;
      case AsmInstruction::Sub: return AsmInstruction::SubIndirect;
      default: return in;
    }
  }
  return in;
}

AssemblyCode CommandBlock::generateMultiplication(IAddress* target, IAddress* op1, IAddress* op2, LabelManager* labelManager){
  AssemblyCode code;
  if (op2->isConstant()){
    // multiplication by constant optimalization
    mpz_class constVal = op2->constValue();
    if (constVal > 0){
      while (constVal > 1){
        mpz_class remainder = constVal % 2;
        if (remainder == 1){
          code.pushToFront(normalizeInstruction(AsmInstruction::Add, op1), op1->getAddress(globalSymbolTable));
        }
        if (constVal > 1){
          code.pushToFront(AsmInstruction::ShiftLeft);
        }
        constVal /= 2;
      }
      code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
      return code;
    }
  }
  // labels
  std::string loopLabel = labelManager->nextLabel("mul_loop");
  std::string tempOddLabel = labelManager->nextLabel("mul_temp2odd");
  std::string oddLabel = labelManager->nextLabel("mul_ifodd");
  std::string odd2Label = labelManager->nextLabel("mul_ifodd2");
  std::string endLabel = labelManager->nextLabel("mul_end");
  // load operands to temp variables and put 0 in target variable
  //code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  if (op1->getAddress(globalSymbolTable) != op2->getAddress(globalSymbolTable)){
    code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::JumpOdd, tempOddLabel);
  code.pushInstruction(AsmInstruction::ShiftRight);

  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::Zero);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));

  // main loop
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP1);
  code.pushLabel(loopLabel);
  code.pushInstruction(AsmInstruction::JumpOdd, oddLabel);
  // if even
  code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::ShiftRight);
  //code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // if odd
  code.pushLabel(oddLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Add, target), target->getAddress(globalSymbolTable));
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));

  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::ShiftRight);
  //code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);

  // if temp2 is odd
  code.pushLabel(tempOddLabel);
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::Zero);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));

  // main loop
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::JumpOdd, odd2Label);
  // if even
  code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::ShiftRight);
  //code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // if odd
  code.pushLabel(odd2Label);

  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Add, target), target->getAddress(globalSymbolTable));
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));

  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::ShiftRight);
  //code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // end
  code.pushLabel(endLabel);

  return code;
}

AssemblyCode CommandBlock::generateDivision(IAddress* target, IAddress* op1, IAddress* op2, LabelManager* labelManager){
  AssemblyCode code;

  if (op2->isConstant()){
    // division by constant optimalization
    mpz_class constVal = op2->constValue();
    // check for division by a power of 2
    mpz_class temp = constVal;
    int shifts = 0;
    while ((temp >> 1) > 0){
      temp = temp >> 1;
      shifts++;
    }
    temp = 1;
    temp = temp << shifts;
    if (temp == constVal){
      // division by bit shifts
      for (int i = 0; i < shifts; i++){
        code.pushInstruction(AsmInstruction::ShiftRight);
      }
      code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
      return code;
    }
  }
  // target = quotient
  // labels
  std::string shiftLoopLabel = labelManager->nextLabel("div_shiftloop");
  std::string firstShiftLoopLabel = labelManager->nextLabel("div_firstshiftloop");
  std::string shiftEndLabel = labelManager->nextLabel("div_shiftend");
  std::string rsmallerLabel = labelManager->nextLabel("div_rsmaller");
  std::string loopLabel = labelManager->nextLabel("div_loop");
  std::string endLabel = labelManager->nextLabel("div_end");
  std::string zeroLabel = labelManager->nextLabel("div_zero");
  bool workOnCopy = (target->getAddress(globalSymbolTable) == op1->getAddress(globalSymbolTable)
                  || target->getAddress(globalSymbolTable) == op2->getAddress(globalSymbolTable));
  // initialize variables
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_REMAINDER);
  // test2
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MOD_POWER);
  // /test2
  code.pushInstruction(AsmInstruction::Zero);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  // division by 0 returns 0
  code.pushInstruction(AsmInstruction::JumpZero, zeroLabel);
  //code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  //code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  // test
  //code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MOD_POWER);
  //code.pushInstruction(AsmInstruction::Increase);
  // /test
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::JumpZero, firstShiftLoopLabel);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);

  code.pushLabel(firstShiftLoopLabel);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);

  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::JumpZero, shiftLoopLabel);
  code.pushInstruction(AsmInstruction::Jump, shiftEndLabel);
  code.pushLabel(shiftLoopLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);

  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::JumpZero, shiftLoopLabel);
  // main loop
  code.pushLabel(shiftEndLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  // main loop body
  code.pushLabel(loopLabel);
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, op2), op2->getAddress(globalSymbolTable));
  // end when temp < b  <=>  temp + 1 <= b
  code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_REMAINDER);
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
  // branch whenever remainder < temp
  code.pushInstruction(AsmInstruction::JumpZero, rsmallerLabel);
  // if remainder >= temp
  code.pushInstruction(AsmInstruction::Decrease);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_REMAINDER);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Load, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Increase);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // if remainder < temp
  code.pushLabel(rsmallerLabel);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Load, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::ShiftLeft);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // special case when division by 0
  code.pushLabel(zeroLabel);
  code.pushInstruction(AsmInstruction::Zero);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_REMAINDER);
  //code.pushInstruction(AsmInstruction::Jump, endLabel);
  // end
  code.pushLabel(endLabel);
  if (workOnCopy) {
    code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }

  return code;
}

AssemblyCode CommandBlock::generateModulo(IAddress* target, IAddress* op1, IAddress* op2, LabelManager* labelManager){
  AssemblyCode code;
  // target = remainder
  // labels
  std::string shiftLoopLabel = labelManager->nextLabel("mod_shiftloop");
  std::string firstShiftLoopLabel = labelManager->nextLabel("div_firstshiftloop");
  std::string shiftEndLabel = labelManager->nextLabel("mod_shiftend");
  std::string rsmallerLabel = labelManager->nextLabel("mod_rsmaller");
  std::string loopLabel = labelManager->nextLabel("mod_loop");
  std::string endLabel = labelManager->nextLabel("mod_end");
  std::string zeroLabel = labelManager->nextLabel("mod_zero");
  bool workOnCopy = (target->getAddress(globalSymbolTable) == op1->getAddress(globalSymbolTable)
                  || target->getAddress(globalSymbolTable) == op2->getAddress(globalSymbolTable));
  // initialize variables
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  // test2
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MOD_POWER);
  // /test2
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  // remainder of division by 0 returns 0
  code.pushInstruction(AsmInstruction::JumpZero, zeroLabel);
  //code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  // shift temp block
  //code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  // test
  //code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MOD_POWER);
  //code.pushInstruction(AsmInstruction::Increase);
  // /test
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::JumpZero, firstShiftLoopLabel);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);

  code.pushLabel(firstShiftLoopLabel);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);

  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::JumpZero, shiftLoopLabel);
  code.pushInstruction(AsmInstruction::Jump, shiftEndLabel);
  code.pushLabel(shiftLoopLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);

  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::JumpZero, shiftLoopLabel);
  // main loop
  code.pushLabel(shiftEndLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  // main loop body
  code.pushLabel(loopLabel);
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, op2), op2->getAddress(globalSymbolTable));
  // end when temp < b  <=>  temp + 1 <= b
  code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Load, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
  // branch whenever remainder >= temp
  code.pushInstruction(AsmInstruction::JumpZero, rsmallerLabel);
  // if remainder >= temp
  code.pushInstruction(AsmInstruction::Decrease);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // if remainder < temp
  code.pushLabel(rsmallerLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // special case when division by 0
  code.pushLabel(zeroLabel);
  code.pushInstruction(AsmInstruction::Zero);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  //code.pushInstruction(AsmInstruction::Jump, endLabel);
  // end
  code.pushLabel(endLabel);
  if (workOnCopy) {
    code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }

  return code;
}

AssemblyCode CommandBlock::getAssembly(LabelManager* labelManager){
  AssemblyCode code;
  for (auto label : labels){
    code.addBlockLabel(label);
  }
  auto iter = commands.begin();

  std::set<std::string> preBlockStates;
  for (auto fromBlock : fromBlocks){
    preBlockStates.insert(fromBlock->postBlockRegisterState());
  }
  std::string registerState = Register::UNDEFINED_REGISTER_STATE;
  mpz_class registerAddress = -1;
  bool isPointer = false;
  bool modified = false;

  // check if block always starts with the same register state
  if (preBlockStates.size() == 1){
    for (auto state : preBlockStates){
      registerState = state;
      if (state != Register::UNDEFINED_REGISTER_STATE){
        registerAddress = globalSymbolTable->getSymbol(state);
      }
    }
  }

  while(iter != commands.end()){
    Command & cmd = (*iter);
    CommandType type = cmd.command;
    if (type == CommandType::Halt){
      code.pushInstruction(AsmInstruction::Halt);
    } else if (type == CommandType::Read){
      code.pushInstruction(AsmInstruction::Get);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::Write){
      // load only when necessary
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
        registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
        isPointer = cmd.addr1.get()->isPointer();
        modified = false;
      }
      code.pushInstruction(AsmInstruction::Put);
    } else if (type == CommandType::Assign){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignAdd){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(normalizeInstruction(AsmInstruction::Add, cmd.addr3.get()), cmd.addr3.get()->getAddress(globalSymbolTable));
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignSub){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, cmd.addr3.get()), cmd.addr3.get()->getAddress(globalSymbolTable));
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignMul){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      AssemblyCode mulBlock = generateMultiplication(cmd.addr1.get(), cmd.addr2.get(), cmd.addr3.get(), labelManager);
      code.pushCode(mulBlock);

      registerAddress = -1;//cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = false;//cmd.addr1.get()->isPointer();
      modified = false;
    } else if (type == CommandType::AssignDiv){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      AssemblyCode divBlock = generateDivision(cmd.addr1.get(), cmd.addr2.get(), cmd.addr3.get(), labelManager);
      code.pushCode(divBlock);

      registerAddress = -1;//cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = false;//cmd.addr1.get()->isPointer();
      modified = false;
    } else if (type == CommandType::AssignMod){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      AssemblyCode modBlock = generateModulo(cmd.addr1.get(), cmd.addr2.get(), cmd.addr3.get(), labelManager);
      code.pushCode(modBlock);

      registerAddress = -1;//cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = false;//cmd.addr1.get()->isPointer();
      modified = false;
    } else if (type == CommandType::Jump){
      std::string lbl = ((AddrLabel*)(*iter).addr1.get())->label;
      code.pushInstruction(AsmInstruction::Jump, lbl);

      registerAddress = -1;//cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = false;//cmd.addr1.get()->isPointer();
      modified = false;
    } else if (type == CommandType::JumpZero){
      //std::cerr << "JZERO " << registerState << " " << cmd.preCommandRegisterState() << std::endl;
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
        registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
        isPointer = cmd.addr1.get()->isPointer();
        modified = false;
      }
      std::string lbl = ((AddrLabel*)(*iter).addr2.get())->label;
      code.pushInstruction(AsmInstruction::JumpZero, lbl);
    } else if (type == CommandType::JumpOdd){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
        registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
        isPointer = cmd.addr1.get()->isPointer();
        modified = false;
      }
      std::string lbl = ((AddrLabel*)(*iter).addr2.get())->label;
      code.pushInstruction(AsmInstruction::JumpOdd, lbl);
    } else if (type == CommandType::Increase){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Increase);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::Decrease){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Decrease);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::ShiftLeft){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::ShiftLeft);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::ShiftRight){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::ShiftRight);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      modified = true;
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    }
    // new register state
    registerState = cmd.postCommandRegisterState();
    // add store instruction only when necessary (register needs to change)
    if (iter == commands.end()){
      // store before end
      if (modified && registerAddress != -1){
        if (isPointer){
          code.pushInstruction(AsmInstruction::StoreIndirect, registerAddress);
        } else {
          if (!globalSymbolTable->hasDontStoreFlag(registerState)){
            code.pushInstruction(AsmInstruction::Store, registerAddress);
          }
        }
      }
    } else if ((*(std::next(iter))).command == CommandType::Jump ||
               (*(std::next(iter))).command == CommandType::JumpZero ||
               (*(std::next(iter))).command == CommandType::JumpOdd ){
      // store before jump
      if (modified && registerAddress != -1){
        if (isPointer){
          code.pushInstruction(AsmInstruction::StoreIndirect, registerAddress);
        } else {
          if (!globalSymbolTable->hasDontStoreFlag(registerState)){
            code.pushInstruction(AsmInstruction::Store, registerAddress);
          }
        }
        modified = false;
      }
    } else if (registerState != (*(std::next(iter))).preCommandRegisterState() ||
               registerState != (*(std::next(iter))).postCommandRegisterState()){
      // store when register state changes
      if (modified && registerAddress != -1){
        if (isPointer){
          code.pushInstruction(AsmInstruction::StoreIndirect, registerAddress);
        } else {
          if (!globalSymbolTable->hasDontStoreFlag(registerState)){
            code.pushInstruction(AsmInstruction::Store, registerAddress);
          }
        }
      }
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

std::string CommandBlock::preBlockRegisterState() const{
  if (commands.empty()){
    return Register::UNDEFINED_REGISTER_STATE;
  } else {
    if (commands[0].command == CommandType::Jump ||
        commands[0].command == CommandType::JumpZero ||
        commands[0].command == CommandType::JumpOdd){
      // jump only block
      return Register::UNDEFINED_REGISTER_STATE;
    } else {
      return commands[0].preCommandRegisterState();
    }
  }
}

std::string CommandBlock::postBlockRegisterState() const{
  if (commands.empty()){
    return Register::UNDEFINED_REGISTER_STATE;
  } else {
    int lastIndex = commands.size() - 1;
    if (commands[lastIndex].command == CommandType::Jump){
          lastIndex--;
    }/* else if (commands[lastIndex].command == CommandType::JumpZero ||
               commands[lastIndex].command == CommandType::JumpOdd){
          return commands[lastIndex].preCommandRegisterState();
    }*/
    if (lastIndex < 0){
      // jump only block
      return Register::UNDEFINED_REGISTER_STATE;
    } else {
      return commands[lastIndex].postCommandRegisterState();
    }
  }
}

std::ostream& operator<<(std::ostream &strm, const CommandBlock &a) {
  strm << "LABELS: ";
  for (auto & label : a.labels){
    strm << label << ", ";
  }
  strm << std::endl;
  strm << "FROM: ";
  for (auto block : a.fromBlocks){
    auto labels = block->labels;
    for (auto & label : labels){
      strm << label << ", ";
    }
    if (!block->commands.empty()){
      strm << "last COMMAND " << block->commands[block->commands.size()-1];
      strm << " REGISTER " << block->postBlockRegisterState();
    }
    strm << std::endl;
  }
  strm << std::endl;
  strm << "REGISTER: " << a.preBlockRegisterState() << std::endl;
  for (auto & command : a.commands){
    strm << command << std::endl;
  }
  strm << "REGISTER: " << a.postBlockRegisterState() << std::endl;
  return strm;
}

void CommandBlock::optimize(){
  // insert optimization here
  // optimize constant additions
  std::vector<Command> newCommands;
  for (auto & command : commands){
    bool emptyReplacement = false;
    std::vector<Command> replacement;
    if (command.command == CommandType::AssignAdd){
      // replace small constant addition with add
      if (command.addr3.get()->isConstant()){
        mpz_class const_val = command.addr3.get()->constValue();
        if (const_val <= 10){
          if (command.addr1.get()->getAddress(globalSymbolTable) != command.addr2.get()->getAddress(globalSymbolTable)){
            replacement.push_back(Command(CommandType::Assign, command.addr1, command.addr2));
          }
          while (const_val > 0){
            replacement.push_back(Command(CommandType::Increase, command.addr1));
            const_val--;
          }
        }
      }
    } else if (command.command == CommandType::AssignSub){
      // replace small constant subtraction with dec
      if (command.addr3.get()->isConstant()){
        mpz_class const_val = command.addr3.get()->constValue();
        if (const_val <= 10){
          if (command.addr1.get()->getAddress(globalSymbolTable) != command.addr2.get()->getAddress(globalSymbolTable)){
            replacement.push_back(Command(CommandType::Assign, command.addr1, command.addr2));
          }
          while (const_val > 0){
            replacement.push_back(Command(CommandType::Decrease, command.addr1));
            const_val--;
          }
        }
      }
    } else if (command.command == CommandType::AssignMul){
      // for multiplication (commutative), put constant as addr3
      if (command.addr2.get()->isConstant()){
        command.addr2.swap(command.addr3);
      }
      if (command.addr3.get()->isConstant()){
        mpz_class const_val = command.addr3.get()->constValue();
        // optimize some multiplication constants
        if (const_val == 0){
          // a * 0 = 0
          std::shared_ptr<IAddress> zeroConst = std::make_shared<AddrConstant>(0);
          replacement.push_back(Command(CommandType::Assign, command.addr1, zeroConst));
        } else if (const_val == 1){
          // a * 1 = a
          if (command.addr1.get()->getAddress(globalSymbolTable) != command.addr2.get()->getAddress(globalSymbolTable)){
            replacement.push_back(Command(CommandType::Assign, command.addr1, command.addr2));
          } else {
            emptyReplacement = true;
          }
        }
      }
    } else if (command.command == CommandType::AssignDiv){
      if (command.addr3.get()->isConstant()){
        mpz_class const_val = command.addr3.get()->constValue();
        // optimize some division constants
        if (const_val == 0){
          // a / 0 = 0
          std::shared_ptr<IAddress> zeroConst = std::make_shared<AddrConstant>(0);
          replacement.push_back(Command(CommandType::Assign, command.addr1, zeroConst));
        } else if (const_val == 1){
          // a / 1 = a
          if (command.addr1.get()->getAddress(globalSymbolTable) != command.addr2.get()->getAddress(globalSymbolTable)){
            replacement.push_back(Command(CommandType::Assign, command.addr1, command.addr2));
          } else {
            emptyReplacement = true;
          }
        }
      }
    } else if (command.command == CommandType::AssignMod){
      if (command.addr3.get()->isConstant()){
        mpz_class const_val = command.addr3.get()->constValue();
        // optimize some modulo constants
        if (const_val == 0 || const_val == 1){
          // a % 0 = 0, a % 1 = 0
          std::shared_ptr<IAddress> zeroConst = std::make_shared<AddrConstant>(0);
          replacement.push_back(Command(CommandType::Assign, command.addr1, zeroConst));
        }
      }
    }
    if (replacement.empty() && !emptyReplacement){
      replacement.push_back(command);
    }
    newCommands.insert(newCommands.end(), replacement.begin(), replacement.end());
  }
  commands = newCommands;
}
