#include "test_utils.hpp"
#include <debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
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

		"Append",

		"Semi",
	  "Colon",
	  "Comma",
	  "LParen",

		"RParen",
	  "LCurlBrace",
	  "RCurlBrace",

		"LSqBrace",
	  "RSqBrace",

		"Arrow",

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

void assert_val_eq(Value expected, Value actual, const char* message) {
	if (expected != actual) {
		fprintf(stderr, "%s: ", message);
		fprintf(stderr, "Expected value to be ");
		print_value(expected);
		fprintf(stderr, " But got ");
		print_value(actual);
		abort();
	}
}

void test_return(const std::string&& code, Value expected, const char* message) {
	VM vm;
	vm.load_stdlib();
	vm.runcode(code);
	assert_val_eq(expected, vm.return_value, message);
}

std::string load_file(const char* filename, bool is_relative) {
	// Currently files can only be read relative to the path of the binary (which is
	// in `bin/vm_test`).
	// So to read the file just from it's name, we append the file beginning of the
	// file path to it.

	static const char* path_prefix = "../test/test_programs/";

	std::string filepath{is_relative ? (std::string(path_prefix) + filename) : filename};
	std::ifstream file(filepath);
	if (file) {
		std::ostringstream stream;
		stream << file.rdbuf();
		return stream.str();
	}

	fprintf(stderr, "Could not open file '%s'", filepath.c_str());
	abort();
}

void test_file(const char* filename, Value expected, const char* message) {
	test_return(load_file(filename), expected, message);
	return;
}

void test_string_return(const char* filename, const char* expected, const char* message) {
	std::string code{load_file(filename)};
	VM vm;
	vm.load_stdlib();

	if (vm.runcode(code) != ExitCode::Success) {
		std::cout << message << "\n";
		abort();
	}

	if (!VYSE_IS_STRING(vm.return_value)) {
		std::cout << "[ Failed ]" << message << "Expected string but got ";
		print_value(vm.return_value);
		std::cout << "\n";
		abort();
	}

	if (std::memcmp(VYSE_AS_CSTRING(vm.return_value), expected, strlen(expected)) != 0) {
		std::cout << message << "[ STRINGS NOT EQUAL ] \n";
		std::cout << "Expected: '" << expected << "'.\n";
		std::cout << "Got: '" << VYSE_AS_CSTRING(vm.return_value) << "'\n";
		abort();
	}
}

void runcode(std::string&& code) {
	VM vm;
	vm.load_stdlib();
	ExitCode ec = vm.runcode(code);
	if (ec != ExitCode::Success) {
		std::cout << "Failure running auto test." << std::endl;
		abort();
	}
}