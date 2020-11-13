#pragma once
#include "ast.hpp"

namespace snap {

class ASTPrinter {
  public:
	ASTPrinter(const std::string* source) : source{source} {};
	void visit(const ASTNode* node) const;

  private:
	const std::string* source;
	u32 m_indent_level = 0;
	const u8 indent_width = 2;

	void visit_program(const Program* program) const;
	void visit_expstmt(const ExprStmt* stmt) const;
	void visit_literal(const Literal* literal) const;
	void visit_node(const ASTNode* node) const;
	void visit_binexpr(const BinExpr* expr) const;
	void visit_unexpr(const UnaryExpr* expr) const;

	void indent();
	void dedent();
};

} // namespace snap