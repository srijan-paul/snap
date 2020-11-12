#pragma once
#include "../syntax/ast/ast.hpp"
#include "type.hpp"
#include <unordered_map>

namespace snap {

class Typechecker {
  public:
	Typechecker(ASTNode* ast) : m_ast{ast} {};
	ASTNode* typecheck();

  private:
	ASTNode* m_ast;
	const Type* check_stmt(Stmt* stmt);
	const Type* check_exp(Expr* exp);
	const Type* check_binexp(BinExpr* exp);
	const Type* check_literal(Literal* literal);
};

} // namespace snap
