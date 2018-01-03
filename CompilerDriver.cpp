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

void CompilerDriver::compile(){
    symbolTable.allocateIterators();
    for(auto symbol : symbolTable.getIterators()){
        symbolTable.printSymbolData(symbol);
    }
    // push halt as final instruction
    Command cmd = Command(CommandType::Halt, nullptr, nullptr, nullptr);
    program.pushCommand(cmd);
    // convert symbols and values to tri-address form
    //cout << programBlock;
    program.convertToTriAddress();
    symbolTable.allocateTempSymbols();
    //cout << "POST CONVERT" << endl;
    cerr << program;
    program.splitToBlocks();

	CommandBlock* p = &program;
	std::set<mpz_class> constants;
	while(p != nullptr){
		std::set<mpz_class> cmd_consts = p->getConstants();
        //std::set<mpz_class> temp = constants;
        //std::set_union(temp.begin(), temp.end(), cmd_consts.begin(), cmd_consts.end(), constants.begin());
		constants.insert(cmd_consts.begin(), cmd_consts.end());
		cerr << *p;
		cerr << "---" << endl;
		p = p->nextBlock.get();
	}
	symbolTable.addConstants(constants);
	symbolTable.allocateConstants();

	p = &program;
	AssemblyCode code = symbolTable.generateConstantsCode();
	while(p != nullptr){
		//cout << "BLOCK" << endl;
		//for(std::string lbl : p->labels){
		//	cout << lbl << ": ";
		//}
		//cout << endl;
		//cout << *p << endl;
		AssemblyCode block = p->getAssembly(&labelManager);
		code.pushCode(block);
		p = p->nextBlock.get();
	}
	cout << code.toString();
}
