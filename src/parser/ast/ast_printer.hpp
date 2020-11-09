#pragma once
#include "ast.hpp"

namespace snap {

class ASTPrinter {
  public:
	ASTPrinter(const std::string& source) : source{source} {};
	void visit(const ASTNode* node) const;
	void visit_literal(const Literal* literal) const;
	void visit_node(const ASTNode* node) const;
	void visit_binexpr(const BinExpr* expr) const;
	void visit_unexpr(const UnaryExpr* expr) const;

  private:
	const std::string& source;
};

} // namespace snap