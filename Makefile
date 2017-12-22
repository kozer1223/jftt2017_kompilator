all: compiler

compiler: compiler.y compiler.l SymbolTable.cpp CommandBlock.cpp Command.cpp LabelManager.cpp Condition.cpp
	flex -o scanner.cpp scanner.l
	bison -o parser.cpp parser.y
	g++ -Wall -o compiler -g main.cpp scanner.cpp parser.cpp CompilerDriver.cpp SymbolTable.cpp CommandBlock.cpp Command.cpp LabelManager.cpp Condition.cpp -lgmpxx -lgmp -std=c++11

clean:
	rm lex.yy.c compiler.tab.c compiler.tab.h compiler
