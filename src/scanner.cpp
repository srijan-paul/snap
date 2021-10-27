#include <cctype>
#include <cstring>
#include <scanner.hpp>

namespace vy {

using TT = TokenType;

Token Scanner::make_token(TT type) const noexcept {
	return Token{type, Location{{start, current - start}, line_pos.line}};
}

Token Scanner::token_if_match(char c, TT then, TT other) {
	if (match(c)) {
		return make_token(then);
	}
	return make_token(other);
}

Token Scanner::next_token() noexcept {
	skip_irrelevant();
	if (eof()) return make_token(TT::Eof);

	start = current;
	const char c = next();

	switch (c) {
	case '+': return token_if_match('=', TT::PlusEq, TT::Plus);
	case '-':
		if (match('>')) return make_token(TT::Arrow);
		return token_if_match('=', TT::MinusEq, TT::Minus);
	case '*':
		if (match('*')) return make_token(TT::Exp);
		return token_if_match('=', TT::MultEq, TT::Mult);

	case '%': return token_if_match('=', TT::ModEq, TT::Mod);
	case '/': return token_if_match('=', TT::DivEq, TT::Div);

	case '=': return token_if_match('=', TT::EqEq, TT::Eq);
	case '!': return token_if_match('=', TT::BangEq, TT::Bang);

	case '>':
		if (match('=')) return make_token(TT::GtEq);
		return token_if_match('>', TT::BitRShift, TT::Gt);

	case '<':
		if (match('=')) return make_token(TT::LtEq);
		if (match('<')) {
			return token_if_match('<', TT::Append, TT::BitLShift);
		} else {
			return make_token(TT::Lt);
		}

	case '&': return token_if_match('&', TT::And, TT::BitAnd);
	case '|': return token_if_match('|', TT::Or, TT::BitOr);
	case '.':
		if (match('.')) {
			return token_if_match('.', TT::DotDotDot, TT::Concat);
		} else {
			return make_token(TT::Dot);
		}

	case '^': return make_token(TT::BitXor);
	case '~': return make_token(TT::BitNot);
	case '#': return make_token(TT::Len);
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
		if (isdigit(c)) return number();

		if (isalpha(c) or c == '_') {
			while (isalnum(peek()) or check('_')) next();
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
	{"false", 5, TT::False}, {"true", 4, TT::True},
	{"nil", 3, TT::Nil},	 {"or", 2, TT::Or},
	{"and", 3, TT::Or},		 {"let", 3, TT::Let},
	{"const", 5, TT::Const}, {"if", 2, TT::If},
	{"else", 4, TT::Else},	 {"while", 5, TT::While},
	{"fn", 2, TT::Fn},		 {"return", 6, TT::Return},
	{"break", 5, TT::Break}, {"continue", 8, TT::Continue},
	{"for", 3, TT::For},
};

TT Scanner::kw_or_id_type() const {
	for (auto& kw : keywords) {
		if (kw.length == (current - start) and
			std::memcmp(source->c_str() + start, kw.word, kw.length) == 0)
			return kw.ttype;
	}
	return TT::Id;
}

// TODO binary, hex, scientific notation
Token Scanner::number() {
	auto type = TT::Integer;
	while (isdigit(peek())) next();
	if (check('.') and isdigit(peek_next())) {
		next(); // consume the '.'
		type = TT::Float;
		while (isdigit(peek())) next();
	}

	return make_token(type);
}

/// TODO: handle unterminated strings.
Token Scanner::make_string(char quote) {
	while (!(eof() or check(quote))) {
		char c = next();
		if (c == '\n') {
			line_pos.line++;
			line_pos.column = 1;
		} else if (c == '\\') {
			// unconditionally consume the next
			// character since it's an escape sequence.
			next();
		}
	}

	if (eof()) return make_token(TT::Error);
	next(); // eat the closing quote.
	return make_token(TT::String);
}

char Scanner::peek() const noexcept {
	if (eof()) return '\0';
	return source->at(current);
}

char Scanner::peek_next() const noexcept {
	if (current + 1 >= source->length()) return '\0';
	return source->at(current + 1);
}

char Scanner::next() {
	return source->at(current++);
}

bool Scanner::eof() const noexcept {
	return current >= source->length() or source->at(current) == '\0';
}

bool Scanner::check(char expected) const noexcept {
	return !eof() and peek() == expected;
}

bool Scanner::match(char expected) {
	if (check(expected)) {
		next();
		return true;
	}
	return false;
}

void Scanner::skip_irrelevant() {
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
		case '-':
			if (peek_next() == '-') {
				skip_comment();
				break;
			}
			return;
		default: return;
		}
	}
}

void Scanner::skip_comment() {
	VYSE_ASSERT(peek() == '-' and peek_next() == '-', "Bad call to Scanner::skip_comment.");
	while (!(eof() or check('\n') or check('\r'))) next();
}

} // namespace vy
