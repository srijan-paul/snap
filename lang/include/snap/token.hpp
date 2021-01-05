#pragma once
#include "common.hpp"
#include <string>

namespace snap {

enum class TokenType {
	// Literals
	Integer = 0,
	Float,
	String,
	True,
	False,

	Id,
	Error,
	Eof,

	// Operators
	Plus,	 // +
	PlusEq,	 // +=
	Concat,	 // ..
	Minus,	 // -
	MinusEq, // -=
	Mult,	 // *
	MultEq,	 // *=
	Div,	 // /
	DivEq,	 // /=
	Mod,	 // %
	ModEq,	 // %=
	Exp,	 // **
	Eq,		 // =
	Bang,	 // !
	Dot,	 // .
	Len,	 // #

	// compare

	Gt,	  // >
	Lt,	  // <
	GtEq, // >=
	LtEq, // <=

	// logic

	And, // &&
	Or,	 // ||

	// Equality

	EqEq,	// ==
	BangEq, // !=

	// bitwise

	BitAnd,	   // &
	BitOr,	   // |
	BitLShift, // >>
	BitRShift, // <<

	// Punctuation
	Semi,		// ;
	Colon,		// :
	Comma,		// ,
	LParen,		// (
	RParen,		// )
	LCurlBrace, // {
	RCurlBrace, // }
	LSqBrace,	// [
	RSqBrace,	// ]

	// Keywords
	Let,
	Const,
	If,
	Else,
	Nil,
	Fn,
	Return,
};

struct SourcePosition {
	u32 start = 0;
	u32 length = 0;
};

struct Location {
	SourcePosition source_pos;
	u32 line = 1;
	u32 column = 0;
};

struct Token {
	TokenType type = TokenType::Error;
	Location location;
	Token(TokenType type, Location loc) : type{type}, location{loc} {};
	Token() : location{} {};
	std::string raw(const std::string& source) const;
	const char* raw_cstr(const std::string* source) const;
	SourcePosition source_pos() const;

	inline u32 length() const {
		return location.source_pos.length;
	}
};

} // namespace snap