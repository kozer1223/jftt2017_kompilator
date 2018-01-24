all: compiler

compiler: compiler.y compiler.l SymbolTable.cpp CommandBlock.cpp Command.cpp LabelManager.cpp Condition.cpp
	flex -o scanner.cpp scanner.l
	bison -o parser.cpp parser.y
	g++ -Wall -Wno-sign-compare -o compiler -g main.cpp scanner.cpp parser.cpp CompilerDriver.cpp SymbolTable.cpp CommandBlock.cpp Command.cpp LabelManager.cpp Condition.cpp -lgmpxx -lgmp -std=c++11

clean:
	rm compiler stack.hh scanner.cpp position.hh parser.hpp parser.cpp location.hh
