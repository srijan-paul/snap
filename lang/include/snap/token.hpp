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
	Plus,		// +
	Concat, // ..
	Minus,	// -
	Mult,		// *
	Div,		// /
	Mod,		// %
	Exp,		// **
	Eq,			// =
	Bang,		// !
	Dot,		// .
	Len,		// #

	// NOTE: It is important that these compound
	// assignment enums are stay in this order.
	// Any new compound assignment operator should be
	// added between `ModEq` and `PlusEq`. This is to make
	// sure that `Compiler::is_assign_tok` works as intended.
	// Refer to `compiler.cpp`.
	ModEq,	 // %=
	DivEq,	 // /=
	MultEq,	 // *=
	MinusEq, // -=
	PlusEq,	 // +=

	// compare

	Gt,		// >
	Lt,		// <
	GtEq, // >=
	LtEq, // <=

	// logic

	And, // &&
	Or,	 // ||

	// Equality

	EqEq,		// ==
	BangEq, // !=

	// bitwise

	BitAnd,		 // &
	BitOr,		 // |
	BitLShift, // >>
	BitRShift, // <<

	// Punctuation
	Semi,				// ;
	Colon,			// :
	Comma,			// ,
	LParen,			// (
	RParen,			// )
	LCurlBrace, // {
	RCurlBrace, // }
	LSqBrace,		// [
	RSqBrace,		// ]

	// Keywords
	Let,
	Const,
	If,
	While,
	Else,
	Nil,
	Fn,
	Return,
	Break
};

struct SourcePosition {
	u32 start = 0;
	u32 length = 0;
};

struct Location {
	SourcePosition source_pos;
	u32 line = 1;
};

struct Token {
	TokenType type = TokenType::Error;
	Location location;
	explicit Token(TokenType type, Location loc) noexcept : type{type}, location{loc} {};
	Token() : location{} {};
	std::string raw(const std::string& source) const;
	const char* raw_cstr(const std::string& source) const;
	SourcePosition source_pos() const;

	inline u32 length() const {
		return location.source_pos.length;
	}
};

} // namespace snap