#ifndef COMPILERDRIVER_H
#define COMPILERDRIVER_H

#include "scanner.h"
#include "parser.hpp"
#include "SymbolTable.hh"
#include "LabelManager.hh"
#include "CommandBlock.hh"

namespace Compiler {

class CompilerDriver {
public:
    CompilerDriver();
    int parse();
    friend class Parser;
    friend class Scanner;

    void compile();

private:
    Scanner scanner;
    Parser parser;
    SymbolTable symbolTable;
    LabelManager labelManager;
    CommandBlock program;
};

}
#endif
