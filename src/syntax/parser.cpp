#include "parser.hpp"
#include "../common.hpp"
#include <iostream>

#define DEFINE_PARSELET(name, condition, next)                                                     \
	Expr* name() {                                                                                 \
		Expr* expr = next();                                                                       \
		while (condition) {                                                                        \
			const Token op_token = token;                                                          \
			expr = new BinExpr(expr, op_token, next());                                            \
		}                                                                                          \
		return expr;                                                                               \
	}

namespace snap {

using TT = TokenType;

Parser::Parser(const std::string* source) : source{source}, scanner{Scanner(source)} {
	advance();
	if (eof()) return;
}

ASTNode* Parser::parse() {
	return program();
}

Program* Parser::program() {
	auto program = new Program();
	while (!eof()) {
		program->stmts.push_back(stmt());
	}
	return program;
}

Stmt* Parser::stmt() {
	Stmt* stmt = new ExprStmt(expression());
	match(TT::Semi); // eat optional semi-colon after each statement
	return stmt;
}

Expr* Parser::expression() {
	return assign();
}

Expr* Parser::assign() {
	Expr* left = logic_or();
	if (match(TT::Eq) || match(TT::PlusEq) || match(TT::MinusEq) || match(TT::MultEq) ||
		match(TT::ModEq) || match(TT::DivEq)) {
		const Token op_token = token;
		left = new BinExpr(left, op_token, assign());
	}
	return left;
}

DEFINE_PARSELET(Parser::logic_or, match(TT::Or), logic_and)
DEFINE_PARSELET(Parser::logic_and, match(TT::And), bit_or)
DEFINE_PARSELET(Parser::bit_or, match(TT::BitOr), bit_and)
DEFINE_PARSELET(Parser::bit_and, match(TT::BitAnd), equality)
DEFINE_PARSELET(Parser::equality, match(TT::EqEq) || match(TT::BangEq), comparison)
DEFINE_PARSELET(Parser::comparison,
				match(TT::Gt) || match(TT::Lt) || match(TT::GtEq) || match(TT::LtEq), bit_shift)
DEFINE_PARSELET(Parser::bit_shift, match(TT::EqEq) || match(TT::BangEq), sum)
DEFINE_PARSELET(Parser::sum, match(TT::Plus) || match(TT::Minus), mult)
DEFINE_PARSELET(Parser::mult, match(TT::Mult) || match(TT::Mod) || match(TT::Div), unary)

Expr* Parser::unary() {
	if (match(TT::Minus) || match(TT::Bang)) {
		const Token op_token = token;
		return new UnaryExpr(op_token, literal());
	}
	return literal();
}

Literal* Parser::literal() {
	advance();
	if (!isLiteral(token.type)) // TODO error
		return nullptr;
	return new Literal(token);
}

#undef DEFINE_PARSELET

// helper functions:

inline bool Parser::isLiteral(TT type) const {
	return (type == TT::Integer || type == TT::String || type == TT::Float);
}

void Parser::advance() {
	prev = token;
	token = peek;
	peek = scanner.next_token();
}

inline bool Parser::eof() const {
	return peek.type == TT::Eof;
}

inline bool Parser::check(TT expected) const {
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