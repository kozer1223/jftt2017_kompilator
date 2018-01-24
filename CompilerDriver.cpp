#include "CompilerDriver.hh"
#include "Command.hh"
#include "AssemblyCode.hh"
#include <sstream>
#include <iostream>

using namespace Compiler;
using namespace std;

CompilerDriver::CompilerDriver(): scanner(*this), parser(scanner, *this)
{}

int CompilerDriver::parse(){
    return parser.parse();
}

/**
 Compile program to stdout.
**/
void CompilerDriver::compile(){
    // allocate iterators
    symbolTable.allocateIterators();
    /** //DEBUG
    for(auto symbol : symbolTable.getIterators()){
        symbolTable.printSymbolData(symbol);
    }**/

    // push halt as final instruction
    Command cmd = Command(CommandType::Halt, nullptr, nullptr, nullptr);
    program.pushCommand(cmd);

    // convert symbols and values to tri-address form
    program.convertToTriAddress();
    symbolTable.allocateTempSymbols();

    //cerr << program; //DEBUG
    // split program into blocks
    program.splitToBlocks();

    // optimize each block
    CommandBlock* p = &program;
    while(p != nullptr){
        p->optimize();
        p = p->nextBlock.get();
    }

    // collect constants
	p = &program;
	std::set<mpz_class> constants;
	while(p != nullptr){
		std::set<mpz_class> cmd_consts = p->getConstants();
		constants.insert(cmd_consts.begin(), cmd_consts.end());
		/* //DEBUG
        cerr << *p;
		cerr << "---" << endl;*/
		p = p->nextBlock.get();
	}
	symbolTable.addConstants(constants);
    // allocate constants
	symbolTable.allocateConstants();

	p = &program;
	AssemblyCode code = symbolTable.generateConstantsCode();
	while(p != nullptr){
		AssemblyCode block = p->getAssembly(&labelManager);
		code.pushCode(block);
		p = p->nextBlock.get();
	}

    // print to cout
	cout << code.toString();
}
