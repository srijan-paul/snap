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
	return expression();
}

Expr* Parser::expression() {
	return sum();
}

Expr* Parser::sum() {
	auto expr = mult();
	while (match(TT::Plus) || match(TT::Minus)) {
		expr = new BinExpr(expr, token, mult());
	}
	return expr;
}

Expr* Parser::mult() {
	Expr* expr = literal();
	while (match(TT::Mult) || match(TT::Div) || match(TT::Mod)) {
		expr = new BinExpr(expr, token, literal());
	}
	return expr;
}

Literal* Parser::literal() {
	advance();
	if (!isLiteral(token.type)) // TODO error
		return nullptr;
	return new Literal(token);
}

// helper functions:

bool Parser::isLiteral(TT type) const {
	return (type == TT::Integer || type == TT::String || type == TT::Float);
}

void Parser::advance() {
	prev = token;
	token = peek;
	peek = scanner.next_token();
}

bool Parser::eof() const {
	return peek.type == TT::Eof;
}

bool Parser::check(TT expected) const {
	return !eof() && peek.type == expected;
}

bool Parser::match(TT expected) {
	if (check(expected)) {
		advance();
		return true;
	}
	return false;
}

}; // namespace snap