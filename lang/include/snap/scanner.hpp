#pragma once
#include "token.hpp"
#include <ctype.h>
#include <stdint.h>
#include <string>

namespace snap {

class Scanner {
	SNAP_NO_COPY(Scanner);
	SNAP_NO_MOVE(Scanner);
	SNAP_NO_DEFAULT_CONSTRUCT(Scanner);

public:
	Scanner(const std::string* src) noexcept : source{src} {};
	Token next_token() noexcept;

private:
	const std::string* source;
	struct {
		u32 line;
		u32 column;
	} line_pos = {1, 1};
	u32 start = 0;
	u32 current = 0;

	void skip_whitespace();
	bool eof() const noexcept;
	char next();
	char peek() const noexcept;
	char peek_next() const noexcept;
	bool check(char expected) const noexcept;
	bool match(char expected);
	inline char current_char() const noexcept {
		return source->at(current - 1);
	}
	inline char lexeme_start() const noexcept {
		return source->at(start);
	}

	TokenType check_kw_chars(const char* rest, u32 offset, u32 length, TokenType ttype) const;
	TokenType kw_or_id_type() const;
	Token make_token(TokenType type) const noexcept;
	Token number();
	Token make_string(char quote);
	Token token_if_match(char c, TokenType then, TokenType other);
};
} // namespace snap