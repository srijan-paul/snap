#include "test_utils.hpp"
#include <debug.hpp>
#include <iostream>
#include <vm.hpp>

using namespace vyse;
using TT = vyse::TokenType;

// clang-format off
void print_ttype(vyse::TokenType type) {
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
	  "BitXor",
	  "BitNot",

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
		"While",
		"For",
	  "Else",
	  "Nil",
	  "Fn",
	  "Return",
	};
	// clang-format on
	const std::string& str = type_strs[static_cast<size_t>(type)];
	std::printf("%-10s", str.c_str());
}

void print_token(const vyse::Token& token, const std::string& src) {
	print_ttype(token.type);
	std::printf("'%s'", token.raw(src).c_str());
}

void print_disassembly(const char* code) {
	const std::string code_s{code};
	vyse::VM vm{&code_s};
	vm.init();
	disassemble_block("test_block", *vm.block());
	printf("\n");
}
