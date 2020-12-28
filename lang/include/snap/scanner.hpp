#pragma once
#include "token.hpp"
#include <ctype.h>
#include <stdint.h>
#include <string>

namespace snap {

class Scanner {
  public:
	Scanner(const std::string* src) : source{src} {};
	Token next_token();
	TokenType kw_or_id_type() const;

  private:
	const std::string* source;
	struct {
		uint32_t line;
		uint32_t column;
	} line_pos = {1, 1};
	uint32_t start = 0;
	uint32_t current = 0;

	void skip_whitespace();
	bool eof() const;
	char next();
	char peek() const;
	char peek_next() const;
	bool check(char expected) const;
	bool match(char expected);
	inline char current_char() const {
		return source->at(current - 1);
	}
	inline char lexeme_start() const {
		return source->at(start);
	}

	TokenType check_kw_chars(const char* rest, u32 offset, u32 length, TokenType ttype) const;
	Token make_token(TokenType type) const;
	Token number();
	Token make_string(char quote);
	Token token_if_match(char c, TokenType then, TokenType other);
};
} // namespace snap