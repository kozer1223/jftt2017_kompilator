#include <iostream>
#include "scanner.h"
#include "parser.hpp"
#include "CompilerDriver.hh"

using namespace Compiler;
using namespace std;

int main(int argc, char **argv) {
    CompilerDriver comp;
    int result = comp.parse();
    if (result == 0){
        comp.compile();
    }
    return result;
}
