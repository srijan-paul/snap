#pragma once
#include <block.hpp>
#include <compiler.hpp>
#include <token.hpp>

void print_ttype(vyse::TokenType ttype);
void print_token(const vyse::Token& token, const std::string& src);
void compile(const std::string* code, vyse::VM* vm);
void print_disassembly(const char* code);

void assert_val_eq(vyse::Value expected, vyse::Value actual, const char* message = "Test failed! ");

std::string load_file(const char* filename, bool is_relative = true);

void test_file(const char* filename, vyse::Value expected, const char* message = "Failure");
/// Given the name of a test file to run, checks the value returned by that file's evaluation.
/// @param filename name of the file, must be inside 'test/test_programs/'.
/// @param expected Expected return value once the file is interpreted.
/// @param message Message to be displayed on failure.
void test_return(const std::string&& code, vyse::Value expected,
								 const char* message = "Test failed!");

/// @brief Runs `filename` and asserts the return value as a cstring, comparing
/// it with `expected`.
void test_string_return(const char* filename, const char* expected,
															 const char* message = "Failed");

/// @brief runs the code and makes sure there is no error.
/// This is used to run automatic tests on files that use 'assert'
/// builtin in vyse to assert program correctness.
void runcode(std::string&& code);

