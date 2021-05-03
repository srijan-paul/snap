#pragma once
#include <block.hpp>
#include <compiler.hpp>
#include <token.hpp>

void print_ttype(vyse::TokenType ttype);
void print_token(const vyse::Token& token, const std::string& src);
void compile(const std::string* code, vyse::VM* vm);
void print_disassembly(const char* code);