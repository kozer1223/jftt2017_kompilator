all: compiler

compiler: compiler.y compiler.l SymbolTable.cpp CommandBlock.cpp Command.cpp LabelManager.cpp Condition.cpp
	bison -d compiler.y
	flex compiler.l
	g++ -Wall -o compiler lex.yy.c compiler.tab.c SymbolTable.cpp CommandBlock.cpp Command.cpp LabelManager.cpp Condition.cpp -lgmpxx -lgmp -std=c++11

clean:
	rm lex.yy.c compiler.tab.c compiler.tab.h compiler
