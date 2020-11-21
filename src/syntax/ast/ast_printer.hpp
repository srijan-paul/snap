#pragma once
#include "ast.hpp"

namespace snap {

class ASTPrinter {
  public:
	ASTPrinter(const std::string* source) : source{source} {};
	void visit(const ASTNode* node);

  private:
	const std::string* source;
	u32 m_indent_level = 0;
	const u8 indent_width = 2;

	void visit_program(const Program* program);
	void visit_expstmt(const ExprStmt* stmt);
	void visit_vardecl(const VarDecl* vdecl);
	void visit_declarator(const Declarator* decl);

	void visit_literal(const Literal* literal);
	void visit_node(const ASTNode* node);
	void visit_binexpr(const BinExpr* expr);
	void visit_unexpr(const UnaryExpr* expr);

	void indent();
	void dedent();
};

} // namespace snap