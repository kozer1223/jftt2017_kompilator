#include "Condition.hh"
#include <utility>

std::vector<Command*> Condition::getCommandBlock(SymbolTable* symbolTable,
  LabelManager* labelManager,
  CommandBlock* trueBlock,
  CommandBlock* falseBlock){

  if (condition == CondType::GreaterEqual){
    // replace with equivalent
    condition = CondType::LessEqual;
    std::swap(lhs, rhs);
  } else if (condition == CondType::Greater) {
    std::swap(trueBlock, falseBlock);
    condition = CondType::LessEqual;
  } else if (condition == CondType::Less) {
    std::swap(lhs, rhs);
    std::swap(trueBlock, falseBlock);
    condition = CondType::LessEqual;
  } else if (condition == CondType::NotEqual) {
    std::swap(trueBlock, falseBlock);
    condition = CondType::Equal;
  }

  bool trueBlockPresent = trueBlock != nullptr;
  bool falseBlockPresent = falseBlock != nullptr;

  std::vector<Command*> commands;
  if (condition == CondType::LessEqual){
    std::string tempSymbol = symbolTable->addTempSymbol();
    std::string trueLabel = labelManager->nextLabel("true_block");
    std::string endLabel = labelManager->nextLabel("end");
    std::string falseLabel = labelManager->nextLabel("false_block");
    commands.push_back(new Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(lhs), new UnparsedValue(rhs)));
    if (trueBlockPresent){
      commands.push_back(new Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(trueLabel)));
    } else {
      commands.push_back(new Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(endLabel)));
    }
    if (falseBlockPresent){
      commands.push_back(new Command(CommandType::Label, new AddrLabel(falseLabel)));
      commands.insert(commands.end(), falseBlock->commands.begin(), falseBlock->commands.end());
      delete falseBlock;
    }
    if (trueBlockPresent){
      commands.push_back(new Command(CommandType::Jump, new AddrLabel(endLabel)));
      commands.push_back(new Command(CommandType::Label, new AddrLabel(trueLabel)));
      commands.insert(commands.end(), trueBlock->commands.begin(), trueBlock->commands.end());
      delete trueBlock;
    }
    commands.push_back(new Command(CommandType::Label, new AddrLabel(endLabel)));
  } else if (condition == CondType::Equal) {
    std::string tempSymbol = symbolTable->addTempSymbol();
    std::string trueLabel = labelManager->nextLabel("true_block");
    std::string endLabel = labelManager->nextLabel("end");
    std::string checkLabel = labelManager->nextLabel("eq_check");
    std::string falseLabel = labelManager->nextLabel("false_block");
    commands.push_back(new Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(lhs), new UnparsedValue(rhs)));
    commands.push_back(new Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(checkLabel)));
    if (falseBlockPresent){
      commands.push_back(new Command(CommandType::Label, new AddrLabel(falseLabel)));
      commands.insert(commands.end(), falseBlock->commands.begin(), falseBlock->commands.end());
      delete falseBlock;
    }
    commands.push_back(new Command(CommandType::Jump, new AddrLabel(endLabel)));
    commands.push_back(new Command(CommandType::Label, new AddrLabel(checkLabel)));
    commands.push_back(new Command(CommandType::AssignSub, new AddrVariable(tempSymbol), new UnparsedValue(rhs), new UnparsedValue(lhs)));
    if (trueBlockPresent){
      commands.push_back(new Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(trueLabel)));
    } else {
      commands.push_back(new Command(CommandType::JumpZero, new AddrVariable(tempSymbol), new AddrLabel(endLabel)));
    }
    if (falseBlockPresent){
      commands.push_back(new Command(CommandType::Jump, new AddrLabel(falseLabel)));
    } else {
      commands.push_back(new Command(CommandType::Jump, new AddrLabel(endLabel)));
    }
    if (trueBlockPresent){
      commands.push_back(new Command(CommandType::Label, new AddrLabel(trueLabel)));
      commands.insert(commands.end(), trueBlock->commands.begin(), trueBlock->commands.end());
      delete trueBlock;
    }
    commands.push_back(new Command(CommandType::Label, new AddrLabel(endLabel)));
  }
  return commands;
}

std::vector<Command*> Condition::getCommandBlock(SymbolTable* symbolTable,
  LabelManager* labelManager,
  CommandBlock* trueBlock){
  return getCommandBlock(symbolTable, labelManager, trueBlock, nullptr);
}
