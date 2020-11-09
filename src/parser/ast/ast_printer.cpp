#include "ast_printer.hpp"
#include <iostream>

namespace snap {

void ASTPrinter::visit(const ASTNode* node) const {
	switch (node->type) {
	case NodeType::Literal: visit_literal((Literal*)node); break;
	default: std::cout << "unknown" << std::endl;
	}
}

void ASTPrinter::visit_literal(const Literal* literal) const {
	std::cout << literal->token.raw(source);
}

void ASTPrinter::visit_binexpr(const BinExpr* expr) const {
	visit(expr->left);
	std::cout << " " << expr->token.raw(source) << " ";
	visit(expr->right);
}

} // namespace snap