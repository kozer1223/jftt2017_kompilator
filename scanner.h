#ifndef SCANNER_H
#define SCANNER_H

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer Compiler_FlexLexer
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL Compiler::Parser::symbol_type Compiler::Scanner::get_next_token()

#include "parser.hpp"

namespace Compiler {
	class CompilerDriver;

    class Scanner : public yyFlexLexer {
    public:
        Scanner(CompilerDriver &driver) : driver(driver) {}
        virtual ~Scanner() {}
        virtual Compiler::Parser::symbol_type get_next_token();
    private:
        CompilerDriver &driver;
    };
}

#endif
