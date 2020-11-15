#include "ast_printer.hpp"
#include <iostream>

#define TAB std::string(m_indent_level, ' ').c_str()

namespace snap {

void ASTPrinter::visit(const ASTNode* node) const {
	switch (node->type) {
	case NodeType::Literal: visit_literal((Literal*)node); break;
	case NodeType::BinExpr: visit_binexpr((BinExpr*)node); break;
	case NodeType::UnaryExpr: visit_unexpr((UnaryExpr*)node); break;
	case NodeType::Program: visit_program((Program*)node); break;
	case NodeType::ExprStmt: visit_expstmt((ExprStmt*)node); break;
	default: std::cout << "unknown" << std::endl;
	}
}

void ASTPrinter::visit_program(const Program* prog) const {
	for (Stmt* stmt : prog->stmts) {
		visit(stmt);
	}
}

void ASTPrinter::visit_expstmt(const ExprStmt* stmt) const {
	std::cout << TAB << "expr stmt: ";
	visit(stmt->exp);
	std::cout << std::endl;
}

void ASTPrinter::visit_literal(const Literal* literal) const {
	std::cout << literal->token.raw(*source);
}

void ASTPrinter::visit_binexpr(const BinExpr* expr) const {
	std::cout << "(";
	visit(expr->left);
	std::cout << " " << expr->token.raw(*source) << " ";
	visit(expr->right);
	std::cout << ")";
}

void ASTPrinter::visit_unexpr(const UnaryExpr* expr) const {
	std::cout << "(";
	std::cout << expr->token.raw(*source);
	visit(expr->operand);
	std::cout << ")";
}

void ASTPrinter::indent() {
	m_indent_level++;
}

void ASTPrinter::dedent() {
	m_indent_level--;
}

} // namespace snap