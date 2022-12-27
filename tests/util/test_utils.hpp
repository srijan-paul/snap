#pragma once
#include <block.hpp>
#include <compiler.hpp>
#include <token.hpp>

void print_ttype(vy::TokenType ttype);
void print_token(const vy::Token& token, const std::string& src);
void compile(const std::string* code, vy::VM* vm);
void print_disassembly(const char* code);

void assert_val_eq(vy::Value expected, vy::Value actual, const char* message = "Test failed! ");

std::string load_file(const char* filename, bool is_relative = true);

void test_file(const char* filename, vy::Value expected, const char* message = "Failure");
/// Given the name of a test file to run, checks the value returned by that file's evaluation.
/// @param filename name of the file, must be inside 'test/test_programs/'.
/// @param expected Expected return value once the file is interpreted.
/// @param message Message to be displayed on failure.
void test_return(const std::string&& code, vy::Value expected,
				 const char* message = "Test failed!");

/// @param code Code for the VyVM to interpret.
/// @param err_msg expected error message when the code is run
void test_error(std::string&& code, const std::string& err_msg);

/// @brief Interprets a file located in `tests/<filepath>` and asserts that it throws
/// <error-message>
/// @param filepath path to the file to interpret.
/// @param error_message expected error message when the file is run
void test_error_in_file(std::string&& filepath, const std::string& error_message);

/// @brief Runs `filename` and asserts the return value as a cstring, comparing
/// it with `expected`.
void test_string_return(const char* filename, const char* expected, const char* message = "Failed");

/// @brief runs the code and makes sure there is no error.
/// This is used to run automatic tests on files that use 'assert'
/// builtin in vyse to assert program correctness.
void runcode(std::string&& code);
