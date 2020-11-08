#pragma once
#include <stdint.h>
#include <string>

namespace snap {

enum class TokenType { //
	Integer = 0,
	Float,
	String,
	Id,
	Error,
	Eof,
	Plus,
	PlusEq,
	Minus,
	MinusEq,
	Mult,
	MultEq,
	Div,
	DivEq,
	Mod,
	ModEq,
	Exp,
	Eq,
	Bang,
	BangEq,
	Semi,
	Dot,
	Colon,
	Comma,
	LParen,
	RParen,
	LCurlBrace,
	RCurlBrace,
	LSqBrace,
	RSqBrace
};

struct SourcePosition {
	uint32_t start = 0;
	uint32_t end = 0;
};

struct Location {
	SourcePosition source_pos;
	uint32_t line = 1;
	uint32_t column = 0;
};

struct Token {
	TokenType type = TokenType::Error;
	Location pos;
};

} // namespace snap