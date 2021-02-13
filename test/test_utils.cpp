#include "test_utils.hpp"
#include <debug.hpp>
#include <iostream>
#include <vm.hpp>

using namespace snap;
using TT = snap::TokenType;

// clang-format off
void print_ttype(snap::TokenType type) {
	std::string type_strs[] = {

		"Integer",	
		"Float",
		"String",
		"True",
		"False",
		"Id",		
		"Error",	  
		"Eof",

		"Plus",		
		"Concat",	  
		"Minus",		
		"Mult",		 
		"Div",
		"Mod",
		"Exp",
		"Eq",
	  "Bang",
	  "Dot",
	  "Len",

		"DivEq",	 
		"ModEq",	 
		"MultEq",	 
		"MinusEq", 
		"PlusEq",

		"Gt",
	  "Lt",
	  "GtEq",
	  "LtEq",

		"And",
	  "Or",
	  "EqEq",
	  "BangEq",

		"BitAnd",
	  "BitOr",
	  "BitLShift",
	  "BitRShift",

		"Semi",
	  "Colon",
	  "Comma",
	  "LParen",

		"RParen",
	  "LCurlBrace",
	  "RCurlBrace",

		"LSqBrace",
	  "RSqBrace",

		"Let",
	  "Const",
	  "If",
	  "Else",
	  "Nil",
	  "Fn",
	  "Return",
	};
	// clang-format on
	const std::string& str = type_strs[static_cast<size_t>(type)];
	std::printf("%-10s", str.c_str());
}

void print_token(const snap::Token& token, const std::string& src) {
	print_ttype(token.type);
	std::printf("'%s'", token.raw(src).c_str());
}

void compile(const std::string* code, VM* vm) {
	Compiler compiler(vm, code);
	compiler.compile();
}

void print_disassembly(const char* code) {
	const std::string code_s{code};
	snap::VM vm{&code_s};
	vm.init();
	disassemble_block("test_block", *vm.block());
	printf("\n");
}
