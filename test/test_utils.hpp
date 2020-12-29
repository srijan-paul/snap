#pragma once
#include <ast_printer.hpp>
#include <block.hpp>
#include <compiler.hpp>
#include <parser.hpp>
#include <token.hpp>

void print_ttype(snap::TokenType ttype);
void print_token(const snap::Token& token, const std::string& src);
void compile(const std::string* code, snap::Block* block);
void print_parsetree(const std::string&& code);
void print_disassembly(const char* code);