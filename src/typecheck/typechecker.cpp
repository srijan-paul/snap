#include "typechecker.hpp"
#include <iostream>
#include <memory>
#include <system_error>

namespace snap {

using BType = BuiltinType;
using TT = TokenType;

#define T_INT	 &BuiltinType::t_int
#define T_FLOAT	 &BuiltinType::t_float
#define T_ERROR	 &BuiltinType::t_error
#define T_STRING &BuiltinType::t_string

Program* Typechecker::typecheck() {
	check_prog();
	return m_ast;
}

void Typechecker::check_prog() {
	for (Stmt* stmt : m_ast->stmts) {
		check_stmt(stmt);
	}
}

const Type* Typechecker::check_stmt(Stmt* stmt) {
	switch (stmt->type) {
	case NodeType::ExprStmt: return check_exp(((ExprStmt*)stmt)->exp);
	default: std::cout << "Typecheck: Unknown statement type." << std::endl; return T_ERROR;
	}
}

const Type* Typechecker::check_exp(Expr* expr) {
	switch (expr->type) {
	case NodeType::Literal: return check_literal((Literal*)expr);
	case NodeType::BinExpr: return check_binexp((BinExpr*)expr);

	default: std::cout << "Type checker: Unexpected expression type." << std::endl;
	}

	return T_ERROR;
}

const Type* Typechecker::check_literal(Literal* literal) {
	const Type* type = nullptr;

	switch (literal->token.type) {
	case TT::Integer: type = T_INT; break;
	case TT::Float: type = T_FLOAT; break;
	case TT::String: type = T_STRING; break;
	default: std::cout << "Typeerror: unknown literal type" << std::endl; type = T_ERROR;
	}

	literal->data_type = type;
	return type;
}

const Type* Typechecker::check_binexp(BinExpr* exp) {
	TT op_type = exp->token.type;

	const Type* ltype = check_exp(exp->left);
	const Type* rtype = check_exp(exp->right);
	const Type* ret_type = T_ERROR;

	switch (op_type) {
	case TT::Minus:
	case TT::Mult:
	case TT::Plus:
		if (!(Type::is_numeric(ltype) && Type::is_numeric(rtype))) {
			ret_type = T_ERROR;
		} else if (ltype == T_FLOAT || rtype == T_FLOAT) {
			ret_type = T_FLOAT;
		} else {
			ret_type = T_INT;
		}
		break;
	case TT::Mod:
	case TT::BitLShift:
		if (ltype != T_INT && rtype != T_INT) {
			ret_type = T_ERROR;
		} else {
			ret_type = T_INT;
		}
		break;
	default: std::cout << "Typeerror: unknown binary operator." << std::endl;
	}

	exp->data_type = ret_type;
	return exp->data_type;
}

} // namespace snap