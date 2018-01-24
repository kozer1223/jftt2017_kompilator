#include "Condition.hh"
#include <utility>

/**
 Generates a command block for a given condition and true/false
 command blocks (used for branching and loops)
**/
std::vector<Command> Condition::getCommandBlock(SymbolTable* symbolTable,
  LabelManager* labelManager,
  CommandBlock& trueBlock,
  CommandBlock& falseBlock){

  // simplify all condition types to <= and =
  if (condition == CondType::GreaterEqual){
    // replace with equivalent
    // a >= b <=> a > b-1
    if (rhs.type == ValueType::VConstant){
      if (rhs.number <= 0){
        // always true
        if (!trueBlock.commands.empty()){
          std::vector<Command> commands;
          commands.insert(commands.end(), trueBlock.commands.begin(), trueBlock.commands.end());
          return commands;
        } else {
          return std::vector<Command>();
        }
      } else {
        rhs.number--;
        std::swap(trueBlock, falseBlock);
        condition = CondType::LessEqual;
      }
    } else {
      condition = CondType::LessEqual;
      std::swap(lhs, rhs);
    }
  } else if (condition == CondType::Greater) {
    std::swap(trueBlock, falseBlock);
    condition = CondType::LessEqual;
  } else if (condition == CondType::Less) {
    // a < b <=> not(b <= a)
    std::swap(lhs, rhs);
    std::swap(trueBlock, falseBlock);
    condition = CondType::LessEqual;
  } else if (condition == CondType::NotEqual) {
    std::swap(trueBlock, falseBlock);
    condition = CondType::Equal;
  }

  bool trueBlockPresent = !trueBlock.commands.empty();
  bool falseBlockPresent = !falseBlock.commands.empty();

  bool isRhsZero = false;

  if (lhs.type == ValueType::VConstant){
    // put constant on rhs
    if (condition == CondType::Equal){
      std::swap(lhs, rhs);
    }
  }

  // constant optimizations
  if (rhs.type == ValueType::VConstant){
    if (rhs.number == 0){
      isRhsZero = true;
      if (condition == CondType::Equal){
        // replace with equivalent
        // a = 0 <=> a <= 0
        condition = CondType::LessEqual;
      } else if (condition == CondType::GreaterEqual){
        // always true
        if (trueBlockPresent){
          std::vector<Command> commands;
          commands.insert(commands.end(), trueBlock.commands.begin(), trueBlock.commands.end());
          return commands;
        } else {
          return std::vector<Command>();
        }
      }
    }
  }

  std::vector<Command> commands;
  if (condition == CondType::LessEqual){
    if (isRhsZero){
      // quick <= 0 optimization
      std::string trueLabel = labelManager->nextLabel("true_block");
      std::string endLabel = labelManager->nextLabel("end");
      std::string falseLabel = labelManager->nextLabel("false_block");
      if (trueBlockPresent){
        commands.push_back(Command(CommandType::JumpZero, new UnparsedValue(lhs), new AddrLabel(trueLabel)));
      } else {
        commands.push_back(Command(CommandType::JumpZero, new UnparsedValue(lhs), new AddrLabel(endLabel)));
      }
      if (falseBlockPresent){
        commands.push_back(Command(CommandType::Label, new AddrLabel(falseLabel)));
        commands.insert(commands.end(), falseBlock.commands.begin(), falseBlock.commands.end());
      }
      if (trueBlockPresent){
        commands.push_back(Command(CommandType::Jump, new AddrLabel(endLabel)));
        commands.push_back(Command(CommandType::Label, new AddrLabel(trueLabel)));
        commands.insert(commands.end(), trueBlock.commands.begin(), trueBlock.commands.end());
      }
      commands.push_back(Command(CommandType::Label, new AddrLabel(endLabel)));
    } else {
      std::string tempSymbol = symbolTable->addTempSymbol(true);
      std::string trueLabel = labelManager->nextLabel("true_block");
      std::string endLabel = labelManager->nextLabel("end");
      std::string falseLabel = labelManager->nextLabel("false_block");
      commands.push_back(Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(lhs), new UnparsedValue(rhs)));
      if (trueBlockPresent){
        commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(trueLabel)));
      } else {
        commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(endLabel)));
      }
      if (falseBlockPresent){
        commands.push_back(Command(CommandType::Label, new AddrLabel(falseLabel)));
        commands.insert(commands.end(), falseBlock.commands.begin(), falseBlock.commands.end());
      }
      if (trueBlockPresent){
        commands.push_back(Command(CommandType::Jump, new AddrLabel(endLabel)));
        commands.push_back(Command(CommandType::Label, new AddrLabel(trueLabel)));
        commands.insert(commands.end(), trueBlock.commands.begin(), trueBlock.commands.end());
      }
      commands.push_back(Command(CommandType::Label, new AddrLabel(endLabel)));
    }
  } else if (condition == CondType::Equal) {
    std::string tempSymbol = symbolTable->addTempSymbol(true);
    std::string trueLabel = labelManager->nextLabel("true_block");
    std::string endLabel = labelManager->nextLabel("end");
    std::string checkLabel = labelManager->nextLabel("eq_check");
    std::string falseLabel = labelManager->nextLabel("false_block");

    if (rhs.type == ValueType::VConstant && rhs.number <= 11) {
      // slightly faster for small constants
      commands.push_back(Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(lhs), new UnparsedValue(rhs)));
      commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(checkLabel)));
      if (falseBlockPresent){
        commands.push_back(Command(CommandType::Label, new AddrLabel(falseLabel)));
        commands.insert(commands.end(), falseBlock.commands.begin(), falseBlock.commands.end());
      }
      commands.push_back(Command(CommandType::Jump, new AddrLabel(endLabel)));
      commands.push_back(Command(CommandType::Label, new AddrLabel(checkLabel)));
      // b <= a <=> a > (b-1)
      mpz_class newRhsNumber = rhs.number - 1;
      if (newRhsNumber < 0){
        newRhsNumber = 0;
      }
      Value newRhs = Value(newRhsNumber);
      if (newRhsNumber == 0){
        // temp symbol is unnecessary
        if (falseBlockPresent){
          commands.push_back(Command(CommandType::JumpZero, new UnparsedValue(lhs), new AddrLabel(falseLabel)));
        } else {
          commands.push_back(Command(CommandType::JumpZero, new UnparsedValue(lhs), new AddrLabel(endLabel)));
        }
      } else {
        commands.push_back(Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(lhs), new UnparsedValue(newRhs)));
        if (falseBlockPresent){
          commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(falseLabel)));
        } else {
          commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(endLabel)));
        }
      }
      if (trueBlockPresent){
        commands.push_back(Command(CommandType::Label, new AddrLabel(trueLabel)));
        commands.insert(commands.end(), trueBlock.commands.begin(), trueBlock.commands.end());
      }
      commands.push_back(Command(CommandType::Label, new AddrLabel(endLabel)));

    } else {
      commands.push_back(Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(lhs), new UnparsedValue(rhs)));
      commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(checkLabel)));
      if (falseBlockPresent){
        commands.push_back(Command(CommandType::Label, new AddrLabel(falseLabel)));
        commands.insert(commands.end(), falseBlock.commands.begin(), falseBlock.commands.end());
      }
      commands.push_back(Command(CommandType::Jump, new AddrLabel(endLabel)));
      commands.push_back(Command(CommandType::Label, new AddrLabel(checkLabel)));
      commands.push_back(Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(rhs), new UnparsedValue(lhs)));
      if (trueBlockPresent){
        commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(trueLabel)));
      } else {
        commands.push_back(Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(endLabel)));
      }
      if (falseBlockPresent){
        commands.push_back(Command(CommandType::Jump, new AddrLabel(falseLabel)));
      } else {
        commands.push_back(Command(CommandType::Jump, new AddrLabel(endLabel)));
      }
      if (trueBlockPresent){
        commands.push_back(Command(CommandType::Label, new AddrLabel(trueLabel)));
        commands.insert(commands.end(), trueBlock.commands.begin(), trueBlock.commands.end());
      }
      commands.push_back(Command(CommandType::Label, new AddrLabel(endLabel)));
    }
  }
  return commands;
}

std::vector<Command> Condition::getCommandBlock(SymbolTable* symbolTable,
  LabelManager* labelManager,
  CommandBlock& trueBlock){
  CommandBlock filler = CommandBlock(symbolTable);
  return getCommandBlock(symbolTable, labelManager, trueBlock, filler);
}
