#pragma once
#include "token.hpp"
#include <stdint.h>
#include <string>

namespace snap {

class Scanner {
  public:
	Scanner(std::string* source);
	Token next_token();

  private:
	std::string* source;
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
	Token make_token(TokenType type) const;
	Token token_if_match(char c, TokenType then, TokenType other);
};
} // namespace snap