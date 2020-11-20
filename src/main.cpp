#include "compiler/compiler.hpp"
#include "debug.hpp"
#include "syntax/ast/ast.hpp"
#include "syntax/ast/ast_printer.hpp"
#include "syntax/parser.hpp"
#include "syntax/scanner.hpp"
#include "token.hpp"
#include "typecheck/typechecker.hpp"
#include "vm/vm.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <stdio.h>

using namespace snap;
using TT = snap::TokenType;
using Op = Opcode;

#define println(s) std::cout << s << std::endl

#define ASSERT(condition, message)                                                                 \
	{                                                                                              \
		if (!(condition)) {                                                                        \
			std::cout << message << std::endl;                                                     \
		}                                                                                          \
		assert(condition);                                                                         \
	}

void assert_equal(Value a, Value b) {
	if (a != b) {
		printf("Expected ");
		print_value(a);
		printf(" but got ");
		print_value(b);
		printf("\n");
	}
	assert(a == b);
}

void print_ttype(TT type) {
	std::string type_strs[] = {
		"Integer",	"Float",	"String",	 "true",	  "false",	"Id",		  "Error",
		"Eof",

		"Plus",		"PlusEq",	"Minus",	 "MinusEq",	  "Mult",	"MultEq",	  "Div",
		"DivEq",	"Mod",		"ModEq",	 "Exp",		  "Eq",		"Bang",		  "Dot",

		"Gt",		"Lt",		"GtEq",		 "LtEq",

		"And",		"Or",

		"EqEq",		"BangEq",

		"BitAnd",	"BitOr",	"BitLShift", "BitRShift",

		"Semi",		"Colon",	"Comma",	 "LParen",	  "RParen", "LCurlBrace", "RCurlBrace",
		"LSqBrace", "RSqBrace", "let",
	};

	const std::string& str = type_strs[(size_t)type];
	printf("%-10s", str.c_str());
}

void assert_token(TT a, TT b) {
	if (a != b) {
		printf("Expected token: ");
		print_ttype(a);
		printf(" Got: ");
		print_ttype(b);
		printf("\n");
	}

	assert(a == b);
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
	println("--- parser test ---");
	std::string s = "1 + 2 - 3 * 4";
	Parser parser{&s};
	auto tree = parser.parse();

	println("code-> " << s << std::endl);
	println("parse tree-> \n");

	ASTPrinter printer(&s);
	printer.visit(tree);

	println("--- /parser test ---\n");
}

void compare_ttypes(const std::string* code, std::vector<TT> expected) {
	Scanner sc{code};
	for (auto tt : expected) {
		assert_token(tt, sc.next_token().type);
	}
}

void lexer_test() {
	std::string code = "123 4.55 + - -= += >= >> << > < <=";
	compare_ttypes(&code, {TT::Integer, TT::Float, TT::Plus, TT::Minus, TT::MinusEq, TT::PlusEq,
						   TT::GtEq, TT::BitRShift, TT::BitLShift, TT::Gt, TT::Lt, TT::LtEq});

	// test keyword and identifier scanning
	code = "let true false xyz";
	compare_ttypes(&code, {TT::Let, TT::True, TT::False, TT::Id, TT::Eof});
}

void compiler_test() {
	println("--- Compiler test ---");

	std::string code = "1 + 2 - 3 * 4;";
	Parser parser(&code);
	auto ast = parser.parse();
	ASTPrinter printer(&code);
	printer.visit(ast);

	Block b;
	Compiler compiler(&b, ast, &code);
	compiler.compile();
	disassemble_block(b);

	println("--- / Compiler test ---");
}

void block_test() {
	println("--- block test ---");

	Block b;
	const u8 index = b.add_value(1.5);
	b.add_instruction(Op::push);
	b.add_num(index);
	b.add_instruction(Op::pop);

	disassemble_block(b);

	println("--- /block test ---\n\n");
}

void vm_test() {
	println("--- VM Tests ---");

	const std::string code = "1 + 2";
	VM vm{&code};
	vm.init();
	vm.step(3);
	assert_equal(vm.peek(0), 3);

	println("--- /VM tests ---");
}

int main() {
	lexer_test();
	parser_test();
	block_test();
	compiler_test();
	vm_test();
	return 0;
}