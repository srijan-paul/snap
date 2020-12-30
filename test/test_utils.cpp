#include "test_utils.hpp"
#include <debug.hpp>
#include <iostream>

using namespace snap;
using TT = snap::TokenType;

void print_ttype(snap::TokenType type) {
	std::string type_strs[] = {

		"Integer",	  "Float",	  "String",	   "True",		"False",

		"Id",		  "Error",	  "Eof",

		"Plus",		  "PlusEq",	  "Concat",	   "Minus",		"MinusEq", "Mult",
		"MultEq",	  "Div",	  "DivEq",	   "Mod",		"ModEq",   "Exp",
		"Eq",		  "Bang",	  "Dot",	   "Len",

		"Gt",		  "Lt",		  "GtEq",	   "LtEq",

		"And",		  "Or",

		"EqEq",		  "BangEq",

		"BitAnd",	  "BitOr",	  "BitLShift", "BitRShift",

		"Semi",		  "Colon",	  "Comma",	   "LParen",	"RParen",  "LCurlBrace",
		"RCurlBrace", "LSqBrace", "RSqBrace",

		"Let",
	};

	const std::string& str = type_strs[static_cast<size_t>(type)];
	std::printf("%-10s", str.c_str());
}

void print_token(const snap::Token& token, const std::string& src) {
	print_ttype(token.type);
	std::printf("'%s'", token.raw(src).c_str());
}

void print_parsetree(const std::string&& code) {
	snap::Parser parser{&code};
	auto tree = parser.parse();

	snap::ASTPrinter printer{&code};
	std::cout << "code: " << code << std::endl;
	printer.visit(tree);
}

void compile(const std::string* code, Block* block) {
	Parser parser(code);
	auto ast = parser.parse();
	ASTPrinter printer(code);
	printer.visit(ast);

	Compiler compiler(block, ast, code);
	compiler.compile();
}

void print_disassembly(const char* code) {
	const std::string code_s{code};
	snap::Block block;
	compile(&code_s, &block);
	disassemble_block(block);
	printf("\n");
}
