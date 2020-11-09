#include "scanner.hpp"
#include "token.hpp"
#include <cctype>
#include <ctype.h>
#include <stdint.h>
#include <string>

namespace snap {

using TT = TokenType;

Token Scanner::make_token(TT type) const {
	return Token{type, Location{{start, current - start}, line_pos.line, line_pos.column}};
}

Token Scanner::token_if_match(char c, TT then, TT other) {
	if (match(c)) {
		return make_token(then);
	}
	return make_token(other);
}

Scanner::Scanner(const std::string* src) {
	source = src;
}

Token Scanner::next_token() {
	if (eof())
		return make_token(TT::Eof);
	skip_whitespace();

	start = current;
	const char c = next();

	switch (c) {
	case '+': return token_if_match('=', TT::PlusEq, TT::Plus);
	case '-': return token_if_match('=', TT::MinusEq, TT::Minus);
	case '*': return token_if_match('=', TT::MultEq, TT::Mult);
	case '%': return token_if_match('=', TT::ModEq, TT::Mod);
	case '/': return token_if_match('=', TT::DivEq, TT::Div);

	case ';': return make_token(TT::Semi);
	case ':': return make_token(TT::Colon);
	case '.': return make_token(TT::Dot);
	case ',': return make_token(TT::Comma);
	case '(': return make_token(TT::LParen);
	case ')': return make_token(TT::RParen);
	case '{': return make_token(TT::LCurlBrace);
	case '}': return make_token(TT::RCurlBrace);
	case '[': return make_token(TT::LSqBrace);
	case ']': return make_token(TT::RSqBrace);

	default:
		if (isdigit(c)) {
			return number();
		}
	}

	return make_token(TT::Error);
}

// TODO binary, hex, scientific notation
Token Scanner::number() {
	auto type = TT::Integer;
	while (isdigit(peek())) next();
	if (match('.')) {
		type = TT::Float;
		while (isdigit(peek())) next();
	}
	return make_token(type);
}

char Scanner::peek() const {
	if (eof())
		return '\0';
	return source->at(current);
}

char Scanner::peek_next() const {
	if (current + 1 >= current)
		return '\0';
	return source->at(current + 1);
}

char Scanner::next() {
	return source->at(current++);
}

bool Scanner::eof() const {
	return current >= source->length() || source->at(current) == '\0';
}

bool Scanner::check(char expected) const {
	return peek() == expected;
}

bool Scanner::match(char expected) {
	if (check(expected)) {
		next();
		return true;
	}
	return false;
}

void Scanner::skip_whitespace() {
	while (true) {
		switch (peek()) {
		case ' ':
		case '\t': next(); break;
		case '\r':
		case '\n':
			line_pos.line++;
			line_pos.column = 1;
			next();
			break;
		default: return;
		}
	}
} // namespace snap

} // namespace snap