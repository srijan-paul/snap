#pragma once
#include "../syntax/ast/ast.hpp"
#include "type.hpp"
#include <unordered_map>

namespace snap {

class Typechecker {
  public:
	Typechecker(Program* ast) : m_ast{ast} {};
	Program* typecheck();

  private:
	Program* m_ast;
	void check_prog();
	const Type* check_stmt(Stmt* stmt);
	const Type* check_exp(Expr* exp);
	const Type* check_binexp(BinExpr* exp);
	const Type* check_literal(Literal* literal);
};

} // namespace snap
