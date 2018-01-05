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
  /*AssemblyCode code;
  // load operands to temp variables and put 0 in target variable
  //code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  if (op1->getAddress(globalSymbolTable) != op2->getAddress(globalSymbolTable)){
    code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::Zero);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  // labels
  std::string loopLabel = labelManager->nextLabel("mul_loop");
  std::string tempOddLabel = labelManager->nextLabel("mul_temp2odd");
  std::string oddLabel = labelManager->nextLabel("mul_ifodd");
  std::string odd2Label = labelManager->nextLabel("mul_ifodd2");
  std::string endLabel = labelManager->nextLabel("mul_end");
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
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // if odd
  code.pushLabel(oddLabel);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, target), target->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::Add, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));

  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP2);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MUL_TEMP1);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);
  // end
  code.pushLabel(endLabel);

  return code;*/
}

AssemblyCode CommandBlock::generateDivision(IAddress* target, IAddress* op1, IAddress* op2, LabelManager* labelManager){
  AssemblyCode code;
  // target = quotient
  // labels
  std::string shiftLoopLabel = labelManager->nextLabel("div_shiftloop");
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
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  // shift temp block
  /*code.pushLabel(shiftLoopLabel);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  // test
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Increase);
  // /test
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::JumpZero, shiftEndLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, shiftLoopLabel);*/
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  // test
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Increase);
  // /test
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::JumpZero, shiftEndLabel);
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
/*
AssemblyCode code;
// target = quotient
// labels
std::string shiftLoopLabel = labelManager->nextLabel("div_shiftloop");
std::string shiftEndLabel = labelManager->nextLabel("div_shiftend");
std::string rbiggerLabel = labelManager->nextLabel("div_rbigger");
std::string loopLabel = labelManager->nextLabel("div_loop");
std::string endLabel = labelManager->nextLabel("div_end");
std::string zeroLabel = labelManager->nextLabel("div_zero");
bool workOnCopy = (target->getAddress(globalSymbolTable) == op1->getAddress(globalSymbolTable)
                || target->getAddress(globalSymbolTable) == op2->getAddress(globalSymbolTable));
// initialize variables
code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_REMAINDER);
code.pushInstruction(AsmInstruction::Zero);
if (workOnCopy){
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
} else {
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
}
code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
// division by 0 returns 0
code.pushInstruction(AsmInstruction::JumpZero, zeroLabel);
code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
// shift temp block
code.pushLabel(shiftLoopLabel);
code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::JumpZero, loopLabel);
code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::ShiftLeft);
code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::Jump, shiftLoopLabel);
// special case when division by 0
code.pushLabel(zeroLabel);
code.pushInstruction(AsmInstruction::Zero);
code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_REMAINDER);
code.pushInstruction(AsmInstruction::Jump, endLabel);
// main loop
code.pushLabel(loopLabel);
code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::Increase);
code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, op2), op2->getAddress(globalSymbolTable));
// end when temp < b  <=>  temp + 1 <= b
code.pushInstruction(AsmInstruction::JumpZero, endLabel);
code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_REMAINDER);
// branch whenever remainder >= temp
code.pushInstruction(AsmInstruction::JumpZero, rbiggerLabel);
// if remainder < temp
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
// if remainder >= temp
code.pushLabel(rbiggerLabel);
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
code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_REMAINDER);
code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_REMAINDER);
code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::ShiftRight);
code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
code.pushInstruction(AsmInstruction::Jump, loopLabel);
// end
code.pushLabel(endLabel);
if (workOnCopy) {
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
}

return code;
*/
}

/*AssemblyCode CommandBlock::generateModulo(IAddress* target, IAddress* op1, IAddress* op2, LabelManager* labelManager){
  AssemblyCode code;
  // target = remainder
  // labels
  std::string loopLabel = labelManager->nextLabel("mod_loop");
  std::string endLabel = labelManager->nextLabel("mod_end");
  std::string ifoddLabel = labelManager->nextLabel("mod_ifodd");
  std::string skip1Label = labelManager->nextLabel("mod_skip1");
  std::string skip2Label = labelManager->nextLabel("mod_skip2");
  std::string skip3Label = labelManager->nextLabel("mod_skip3");
  std::string zeroLabel = labelManager->nextLabel("mod_zero");
  bool workOnCopy = (target->getAddress(globalSymbolTable) == op1->getAddress(globalSymbolTable)
                  || target->getAddress(globalSymbolTable) == op2->getAddress(globalSymbolTable));
  // initialize variables
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Zero);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MOD_POWER);

  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op2), op2->getAddress(globalSymbolTable));
  // remainder of division by 0 or 1 returns 0
  code.pushInstruction(AsmInstruction::JumpZero, zeroLabel);
  code.pushInstruction(AsmInstruction::Decrease);
  code.pushInstruction(AsmInstruction::JumpZero, zeroLabel);

  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  // main loop
  code.pushLabel(loopLabel);
  code.pushInstruction(AsmInstruction::JumpZero, endLabel);
  code.pushInstruction(AsmInstruction::JumpOdd, ifoddLabel);
  // if even
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, op2), op2->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::JumpZero, skip1Label);
  code.pushInstruction(AsmInstruction::Decrease);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MOD_POWER);

  // shift temp
  code.pushLabel(skip1Label);
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
  code.pushInstruction(AsmInstruction::Jump, endLabel);

  // if odd
  code.pushLabel(ifoddLabel);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Load, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Add, globalSymbolTable->MOD_POWER);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, op2), op2->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::JumpZero, skip2Label);
  code.pushInstruction(AsmInstruction::Decrease);
  if (workOnCopy){
    code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_COPY);
  } else {
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }

  // if odd part 2
  code.pushLabel(skip2Label);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MOD_POWER);
  code.pushInstruction(AsmInstruction::Increase);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, op2), op2->getAddress(globalSymbolTable));
  code.pushInstruction(AsmInstruction::JumpZero, skip3Label);
  code.pushInstruction(AsmInstruction::Decrease);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->MOD_POWER);

  // shift temp
  code.pushLabel(skip3Label);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, loopLabel);

  code.pushLabel(endLabel);
  if (workOnCopy) {
    code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_COPY);
    code.pushInstruction(normalizeInstruction(AsmInstruction::Store, target), target->getAddress(globalSymbolTable));
  }
  return code;
}*/

AssemblyCode CommandBlock::generateModulo(IAddress* target, IAddress* op1, IAddress* op2, LabelManager* labelManager){
  AssemblyCode code;
  // target = remainder
  // labels
  std::string shiftLoopLabel = labelManager->nextLabel("mod_shiftloop");
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
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  // shift temp block
  /*code.pushLabel(shiftLoopLabel);
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  // test
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Increase);
  // /test
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::JumpZero, shiftEndLabel);
  code.pushInstruction(AsmInstruction::Load, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::ShiftLeft);
  code.pushInstruction(AsmInstruction::Store, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::Jump, shiftLoopLabel);*/
  code.pushInstruction(normalizeInstruction(AsmInstruction::Load, op1), op1->getAddress(globalSymbolTable));
  // test
  code.pushInstruction(AsmInstruction::ShiftRight);
  code.pushInstruction(AsmInstruction::Increase);
  // /test
  code.pushInstruction(AsmInstruction::Sub, globalSymbolTable->DIV_TEMP);
  code.pushInstruction(AsmInstruction::JumpZero, shiftEndLabel);
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
  std::string registerState = Register::UNDEFINED_REGISTER_STATE;
  mpz_class registerAddress = -1;
  bool isPointer = false;
  while(iter != commands.end()){
    Command & cmd = (*iter);
    CommandType type = cmd.command;
    if (type == CommandType::Halt){
      code.pushInstruction(AsmInstruction::Halt);
    } else if (type == CommandType::Read){
      code.pushInstruction(AsmInstruction::Get);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::Write){
      // load only when necessary
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
        registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
        isPointer = cmd.addr1.get()->isPointer();
      }
      code.pushInstruction(AsmInstruction::Put);
    } else if (type == CommandType::Assign){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignAdd){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(normalizeInstruction(AsmInstruction::Add, cmd.addr3.get()), cmd.addr3.get()->getAddress(globalSymbolTable));
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignSub){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(normalizeInstruction(AsmInstruction::Sub, cmd.addr3.get()), cmd.addr3.get()->getAddress(globalSymbolTable));
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::AssignMul){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      AssemblyCode mulBlock = generateMultiplication(cmd.addr1.get(), cmd.addr2.get(), cmd.addr3.get(), labelManager);
      code.pushCode(mulBlock);

      registerAddress = -1;//cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = false;//cmd.addr1.get()->isPointer();
    } else if (type == CommandType::AssignDiv){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      AssemblyCode divBlock = generateDivision(cmd.addr1.get(), cmd.addr2.get(), cmd.addr3.get(), labelManager);
      code.pushCode(divBlock);

      registerAddress = -1;//cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = false;//cmd.addr1.get()->isPointer();
    } else if (type == CommandType::AssignMod){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr2.get()), cmd.addr2.get()->getAddress(globalSymbolTable));
      }
      AssemblyCode modBlock = generateModulo(cmd.addr1.get(), cmd.addr2.get(), cmd.addr3.get(), labelManager);
      code.pushCode(modBlock);

      registerAddress = -1;//cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = false;//cmd.addr1.get()->isPointer();
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
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Increase);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::Decrease){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::Decrease);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::ShiftLeft){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::ShiftLeft);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    } else if (type == CommandType::ShiftRight){
      if (registerState != cmd.preCommandRegisterState()){
        code.pushInstruction(normalizeInstruction(AsmInstruction::Load, cmd.addr1.get()), cmd.addr1.get()->getAddress(globalSymbolTable));
      }
      code.pushInstruction(AsmInstruction::ShiftRight);
      registerAddress = cmd.addr1.get()->getAddress(globalSymbolTable);
      isPointer = cmd.addr1.get()->isPointer();
      //code.pushInstruction(AsmInstruction::Store, cmd.addr1.get()->getAddress(globalSymbolTable));
    }
    // add store instruction only when necessary (register needs to change)
    if (iter == commands.end()){
      //std::cerr << cmd << std::endl;
      if (registerAddress != -1){
        if (isPointer){
          code.pushInstruction(AsmInstruction::StoreIndirect, registerAddress);
        } else {
          code.pushInstruction(AsmInstruction::Store, registerAddress);
        }
      }
    } else if (cmd.postCommandRegisterState() != (*(std::next(iter))).preCommandRegisterState()){
      //std::cerr << cmd << std::endl;
      //std::cerr << cmd.postCommandRegisterState() << " -> " << (*(std::next(iter))).preCommandRegisterState() << std::endl;
      if (registerAddress != -1){
        if (isPointer){
          code.pushInstruction(AsmInstruction::StoreIndirect, registerAddress);
        } else {
          code.pushInstruction(AsmInstruction::Store, registerAddress);
        }
      }
    } else if (cmd.postCommandRegisterState() != (*(std::next(iter))).postCommandRegisterState()){
      //std::cerr << cmd << std::endl;
      //std::cerr << cmd.postCommandRegisterState() << " -> " << (*(std::next(iter))).postCommandRegisterState() << std::endl;
      if (registerAddress != -1){
        if (isPointer){
          code.pushInstruction(AsmInstruction::StoreIndirect, registerAddress);
        } else {
          code.pushInstruction(AsmInstruction::Store, registerAddress);
        }
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

void CommandBlock::optimize(){
  // insert optimization here
  // optimize constant additions
  std::vector<Command> newCommands;
  for (auto & command : commands){
    std::vector<Command> replacement;
    if (command.command == CommandType::AssignAdd){
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
    }
    if (replacement.empty()){
      replacement.push_back(command);
    }
    newCommands.insert(newCommands.end(), replacement.begin(), replacement.end());
  }
  commands = newCommands;
}
