#pragma once
#include <block.hpp>
#include <compiler.hpp>
#include <token.hpp>

void print_ttype(snap::TokenType ttype);
void print_token(const snap::Token& token, const std::string& src);
void compile(const std::string* code, snap::VM* vm);
void print_disassembly(const char* code);