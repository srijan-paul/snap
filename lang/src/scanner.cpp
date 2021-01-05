#include <cctype>
#include <cstring>
#include <scanner.hpp>
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

Token Scanner::next_token() {
	skip_whitespace();
	if (eof()) return make_token(TT::Eof);

	start = current;
	const char c = next();

	switch (c) {
	case '+': return token_if_match('=', TT::PlusEq, TT::Plus);
	case '-': return token_if_match('=', TT::MinusEq, TT::Minus);
	case '*': return token_if_match('=', TT::MultEq, TT::Mult);
	case '%': return token_if_match('=', TT::ModEq, TT::Mod);
	case '/': return token_if_match('=', TT::DivEq, TT::Div);

	case '=': return token_if_match('=', TT::EqEq, TT::Eq);
	case '!': return token_if_match('=', TT::BangEq, TT::Bang);

	case '>':
		if (match('=')) return make_token(TT::GtEq);
		if (match('>')) return make_token(TT::BitRShift);
		return make_token(TT::Gt);
	case '<':
		if (match('=')) return make_token(TT::LtEq);
		if (match('<')) return make_token(TT::BitLShift);
		return make_token(TT::Lt);

	case '&': return token_if_match('&', TT::And, TT::BitAnd);
	case '|': return token_if_match('|', TT::Or, TT::BitOr);
	case '.': return token_if_match('.', TT::Concat, TT::Dot);

	case ';': return make_token(TT::Semi);
	case ':': return make_token(TT::Colon);
	case ',': return make_token(TT::Comma);
	case '(': return make_token(TT::LParen);
	case ')': return make_token(TT::RParen);
	case '{': return make_token(TT::LCurlBrace);
	case '}': return make_token(TT::RCurlBrace);
	case '[': return make_token(TT::LSqBrace);
	case ']': return make_token(TT::RSqBrace);
	case '\'':
	case '"': return make_string(c);
	default:
		if (isdigit(c)) {
			return number();
		}

		if (isalpha(c) || c == '_') {
			while (isalnum(peek()) || check('_')) next();
			return make_token(kw_or_id_type());
		}
	}

	return make_token(TT::Error);
}

TT Scanner::check_kw_chars(const char* rest, u32 kwlen, u32 cmplen, TT ttype) const {
	u32 offset = kwlen - cmplen;
	if (kwlen != (current - start)) return TT::Id;
	const char* compare_from = source->c_str() + start + offset;
	if (memcmp(compare_from, rest, cmplen) == 0) return ttype;
	return TT::Id;
}

typedef struct {
	const char* word;
	u32 length;
	TT ttype;
} KeywordData;

static constexpr KeywordData keywords[] = {
	{"false", 5, TT::False}, {"true", 4, TT::True}, {"nil", 3, TT::Nil},	   {"or", 2, TT::Or},
	{"and", 3, TT::Or},		 {"let", 3, TT::Let},	{"const", 5, TT::Const},   {"if", 2, TT::If},
	{"else", 2, TT::Else},	 {"fn", 2, TT::Fn},		{"return", 6, TT::Return},
};

TT Scanner::kw_or_id_type() const {
	for (auto& kw : keywords) {
		if (kw.length == (current - start) &&
			std::memcmp(source->c_str() + start, kw.word, kw.length) == 0)
			return kw.ttype;
	}
	return TT::Id;
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

// TODO: escape characters.
Token Scanner::make_string(char quote) {
	while (!(eof() || check(quote))) next();
	if (eof()) {
		// TODO: error
	} else {
		next(); // eat the closing quote.
	}
	return make_token(TT::String);
}

char Scanner::peek() const {
	if (eof()) return '\0';
	return source->at(current);
}

char Scanner::peek_next() const {
	if (current + 1 >= current) return '\0';
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
		case '\r':
		case '\t': next(); break;
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