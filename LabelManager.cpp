#include "LabelManager.hh"
#include <iostream>
#include <sstream>

LabelManager::LabelManager(){
  lastLabelIndex = 0;
}

LabelManager::~LabelManager(){}

/**
 Generates a new, unique label with a given infix (for debug purposes).
**/
std::string LabelManager::nextLabel(std::string infix){
  std::stringstream ss;
  ss << "LABEL_";
  ss << infix << "_";
  lastLabelIndex++;
  ss << lastLabelIndex;

  std::string newLabel = ss.str();
  labelSet.insert(newLabel);
  return newLabel;
}

std::string LabelManager::nextLabel(){
  return nextLabel("");
}
