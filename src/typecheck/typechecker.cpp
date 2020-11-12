#include "typechecker.hpp"
#include <iostream>
#include <memory>
#include <system_error>

namespace snap {

using BType = BuiltinType;
using TT = TokenType;

#define T_INT	 BuiltinType::t_int
#define T_FLOAT	 BuiltinType::t_float
#define T_ERROR	 BuiltinType::t_error
#define T_STRING BuiltinType::t_string

ASTNode* Typechecker::typecheck() {
	return m_ast;
}

const Type* Typechecker::check_stmt(Stmt* stmt) {
	switch (stmt->type) {
	case NodeType::ExprStmt: return check_exp(((ExprStmt*)stmt)->exp);
	default: return &T_ERROR;
	}
}

const Type* Typechecker::check_exp(Expr* expr) {
	switch (expr->type) {
	case NodeType::Literal: return check_literal((Literal*)expr);
	case NodeType::BinExpr: return check_binexp((BinExpr*)expr);

	default: std::cout << "Type checker: Unexpected expression type." << std::endl;
	}

	return &T_ERROR;
}

const Type* Typechecker::check_literal(Literal* literal) {
	switch (literal->token.type) {
	case TT::Integer: return &T_INT;
	case TT::Float: return &T_FLOAT;
	case TT::String: return &T_STRING;
	default: return &T_ERROR;
	}
}

const Type* Typechecker::check_binexp(BinExpr* exp) {
	TT op_type = exp->token.type;

	const Type* ltype = check_exp(exp->left);
	const Type* rtype = check_exp(exp->right);

	switch (op_type) {
	case TT::Minus:
	case TT::Mult:
	case TT::Plus:
		if (!(Type::is_numeric(ltype) && Type::is_numeric(rtype))) return &T_ERROR;
		if (ltype == &T_FLOAT || rtype == &T_FLOAT) return &T_FLOAT;
		return &T_INT;
	case TT::Mod:
	case TT::BitLShift:
		if (ltype != &T_INT && rtype != &T_INT) return &T_ERROR;
		return &T_INT;
	default: return &T_ERROR;
	}
}

} // namespace snap