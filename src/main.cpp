#include "compiler/compiler.hpp"
#include "syntax/ast/ast.hpp"
#include "syntax/ast/ast_printer.hpp"
#include "syntax/parser.hpp"
#include "syntax/scanner.hpp"
#include "token.hpp"
#include "typecheck/typechecker.hpp"


#include "debug.hpp"
#include <cstdio>
#include <stdio.h>


using namespace snap;
using TT = snap::TokenType;
using Op = Opcode;

#define println(s) std::cout << s << std::endl

void print_ttype(TT type) {
	std::string type_strs[] = {
		"Integer",	"Float",   "String",	"Id",		 "Error",  "Eof",

		"Plus",		"PlusEq",  "Minus",		"MinusEq",	 "Mult",   "MultEq",	 "Div",
		"DivEq",	"Mod",	   "ModEq",		"Exp",		 "Eq",	   "Bang",		 "Dot",

		"Gt",		"Lt",	   "GtEq",		"LtEq",

		"And",		"Or",

		"EqEq",		"BangEq",

		"BitAnd",	"BitOr",   "BitLShift", "BitRShift",

		"Semi",		"Colon",   "Comma",		"LParen",	 "RParen", "LCurlBrace", "RCurlBrace",
		"LSqBrace", "RSqBrace"};

	const std::string& str = type_strs[(size_t)type];
	printf("%-10s", str.c_str());
}

void print_token(const Token& token, const std::string& src) {
	print_ttype(token.type);
	printf("'%s'", token.raw(src).c_str());
	printf("\n");
}

void checker_test() {
	std::string code = "1 + 2";
	Parser parser{&code};
	auto ast = parser.parse();
	Typechecker checker{ast};
	checker.typecheck();
}

void parser_test() {
	std::string s = "112 + 25.4; 2 + 3; 4 * -5 - 1 * 3";
	Parser parser{&s};
	auto tree = parser.parse();

	ASTPrinter printer{&s};
	printer.visit(tree);
}

void lexer_test() {
	std::string s = "123 4.55 + - -= += >= >> << > < <=";
	Scanner sc{&s};
	while (true) {
		const Token token = sc.next_token();
		print_token(token, s);
		if (token.type == TT::Eof) break;
	}

	printf("\n ======== \n\n");
}

void compiler_test() {
	std::string code = "1 + 2;";
}

void block_test() {

	println("--- block test ---");

	Block b;

	const u8 index = b.add_value(1.5);
	b.add_instruction(Op::push);
	b.add_num(index);
	b.add_instruction(Op::pop);

	disassemble_block(b);

	println("--- /block test ---");
}

int main() {
	lexer_test();
	parser_test();
	block_test();
	return 0;
}