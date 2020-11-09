#include "parser.hpp"
#include "../common.hpp"
#include <iostream>

namespace snap {

using TT = TokenType;

Parser::Parser(const std::string* source) : source{source}, scanner{Scanner(source)} {
	advance();
	if (eof())
		return;
}

ASTNode* Parser::parse() {
	return literal();
}

Literal* Parser::literal() {
	advance();
	return new Literal(token);
}

void Parser::advance() {
	prev = token;
	token = peek;
	peek = scanner.next_token();
}

bool Parser::eof() const {
	return peek.type == TT::Eof;
}

}; // namespace snap