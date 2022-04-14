#include "test_utils.hpp"
#include "../assert.hpp"
#include "value.hpp"
#include <debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vm.hpp>

using namespace vy;
using TT = vy::TokenType;

void print_ttype(vy::TokenType type) {
	std::string type_strs[] = {
		"Integer",	 "Float",	   "String",
		"True",		 "False",

		"Id",		 "Error",	   "ErrStringTerminate",
		"Eof",

		"Plus",		 "Concat",	   "Minus",
		"Mult",		 "Div",		   "Mod",
		"Exp",		 "Eq",		   "Bang",
		"Dot",		 "DotDotDot",  "Len",

		"DivEq",	 "ModEq",	   "MultEq",
		"MinusEq",	 "PlusEq",

		"Gt",		 "Lt",		   "GtEq",
		"LtEq",

		"And",		 "Or",		   "EqEq",
		"BangEq",

		"BitAnd",	 "BitOr",	   "BitLShift",
		"BitRShift", "BitXor",	   "BitNot",

		"Append",

		"Semi",		 "Colon",	   "Comma",
		"LParen",

		"RParen",	 "LCurlBrace", "RCurlBrace",

		"LSqBrace",	 "RSqBrace",

		"Arrow",

		"Let",		 "Const",	   "If",
		"While",	 "For",		   "Else",
		"Nil",		 "Fn",		   "Return",
		"Break",	 "Continue",
	};
	const std::string& str = type_strs[static_cast<size_t>(type)];
	std::printf("%-10s", str.c_str());
}

void print_token(const vy::Token& token, const std::string& src) {
	print_ttype(token.type);
	std::printf("'%s'", token.raw(src).c_str());
}

void print_disassembly(const char* code) {
	vy::VM vm{code};
	vm.init();
	disassemble_block("test_block", *vm.block());
	printf("\n");
}

void assert_val_eq(Value expected, Value actual, const char* message) {
	if (expected != actual) {
		fprintf(stderr, "%s: ", message);
		fprintf(stderr, "Expected value to be %s", value_to_string(expected).c_str());
		fprintf(stderr, " But got %s\n", value_to_string(actual).c_str());
		abort();
	}
}

void test_return(const std::string&& code, Value expected, const char* message) {
	VM vm;
	vm.load_stdlib();
	vm.runcode(code);
	assert_val_eq(expected, vm.return_value, message);
}

void test_error(const std::string&& code, const std::string& message) {
	VM vm;
	vm.load_stdlib();
	vm.on_error = [](VM& vm, RuntimeError error) {
		vm.set_global("#ErrMsg#",
					  VYSE_OBJECT(&vm.make_string(error.message.c_str(), error.message.length())));
	};

	vm.runcode(code);
	const Value err_msg = vm.get_global(&vm.make_string("#ErrMsg#"));
	const std::string fail_str = "Expected error: " + message;
	ASSERT(VYSE_IS_STRING(err_msg) && message == VYSE_AS_STRING(err_msg)->c_str(),
		   "Expected error");
}

std::string load_file(const char* filename, bool is_relative) {
	// Currently files can only be read relative to the path of the binary (which is
	// in `build/vm_test`).
	// So to read the file just from it's name, we append the file beginning of the
	// file path to it.

	static const char* path_prefix = "../tests/test_programs/";

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