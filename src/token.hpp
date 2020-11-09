#pragma once
#include "common.hpp"
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
	u32 start = 0;
	u32 end = 0;
};

struct Location {
	SourcePosition source_pos;
	u32 line = 1;
	u32 column = 0;
};

struct Token {
  private:
	Location location;

  public:
	TokenType type = TokenType::Error;
	Token(TokenType type, Location loc) : type{type}, location{loc} {};
	std::string raw(const std::string& source) const;
	SourcePosition source_pos() const;
};

} // namespace snap