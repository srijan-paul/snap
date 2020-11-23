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

Program* Parser::parse() {
	return program();
}

Program* Parser::program() {
	auto program = new Program();
	while (!eof()) {
		program->stmts.push_back(declaration());
	}
	return program;
}

Stmt* Parser::declaration() {
	if (match(TT::Let)) {
		return var_decl();
	}
	match(TT::Semi);
	return stmt();
}

VarDecl* Parser::var_decl() {
	VarDecl* vdecl = new VarDecl();
	do {
		vdecl->declarators.push_back(var_declarator());
	} while (match(TT::Comma));
	return vdecl;
}

Declarator* Parser::var_declarator() {
	expect(TT::Id, "Expected variable name.");
	Declarator* decl = new Declarator(token);
	if (match(TT::Eq)) {
		decl->init = expression();
	}
	return decl;
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
	// TODO check if LHS is valid assignment target.
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
		return new UnaryExpr(op_token, primary());
	}
	return primary();
}

Expr* Parser::primary() {
	advance();
	if (isLiteral(token.type)) return new Literal(token);
	if (token.type == TT::Id) return new VarId(token);
	// TODO error
	return nullptr;
}

#undef DEFINE_PARSELET

// helper methods:

void Parser::advance() {
	prev = token;
	token = peek;
	peek = scanner.next_token();
}

bool Parser::match(TT expected) {
	if (check(expected)) {
		advance();
		return true;
	}
	return false;
}

void Parser::expect(TT expected, const char* err_msg) {
	if (check(expected)) {
		advance();
		return;
	}

	error(err_msg);
}

void Parser::error(const char* message) {
	std::cout << message << std::endl;
}

}; // namespace snap