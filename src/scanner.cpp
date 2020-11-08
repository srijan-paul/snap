#include "scanner.hpp"
#include "token.hpp"
#include <stdint.h>
#include <string>

namespace snap {

using TT = TokenType;

Token Scanner::make_token(TT type) const {
	return Token{type, Location{{start, current}, line_pos.line, line_pos.column}};
}

Token Scanner::token_if_match(char c, TT then, TT other) {
	if (match(c)) {
		return make_token(then);
	}
	return make_token(other);
}

Scanner::Scanner(std::string* src) {
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
	case '/': return token_if_match('/', TT::DivEq, TT::Div);
	}

	return make_token(TT::Error);
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
}

} // namespace snap