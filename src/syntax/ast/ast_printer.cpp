#include "ast_printer.hpp"
#include <iostream>

#define TAB std::string(m_indent_level, ' ').c_str()

namespace snap {

static void print_var(const VarId* var, const std::string* source) {
	std::cout << var->token.raw(*source);
}

void ASTPrinter::visit(const ASTNode* node) {
	// clang-format off
	switch (node->type) {
	case NodeType::Literal:        visit_literal((Literal*)node);   break;
	case NodeType::VarId:          print_var((VarId*)node, source); break;
	case NodeType::BinExpr:        visit_binexpr((BinExpr*)node);   break;
	case NodeType::UnaryExpr:      visit_unexpr((UnaryExpr*)node);  break;
	case NodeType::Program:        visit_program((Program*)node);   break;
	case NodeType::ExprStmt:  	   visit_expstmt((ExprStmt*)node);  break;
	case NodeType::VarDeclaration: visit_vardecl((VarDecl*)node);   break;
	default: std::cout << "<< unknown AST Node type >>" << std::endl;
	}
	// clang-format on
}

void ASTPrinter::visit_program(const Program* prog) {
	for (Stmt* stmt : prog->stmts) {
		visit(stmt);
	}
}

void ASTPrinter::visit_vardecl(const VarDecl* vdecl) {
	std::cout << TAB << "let \n";
	indent();
	for (auto d : vdecl->declarators) {
		visit_declarator(d);
	}
	dedent();
}

void ASTPrinter::visit_declarator(const Declarator* decl) {
	std::cout << TAB << decl->var.raw(*source);
	if (decl->init != nullptr) {
		std::cout << " = ";
		visit(decl->init);
	}
	std::cout << "," << std::endl;
}

void ASTPrinter::visit_expstmt(const ExprStmt* stmt) {
	std::cout << TAB << "expr stmt: ";
	visit(stmt->exp);
	std::cout << std::endl;
}

void ASTPrinter::visit_literal(const Literal* literal) const {
	std::cout << literal->token.raw(*source);
}

void ASTPrinter::visit_binexpr(const BinExpr* expr) {
	std::cout << "(";
	visit(expr->left);
	std::cout << " " << expr->token.raw(*source) << " ";
	visit(expr->right);
	std::cout << ")";
}

void ASTPrinter::visit_unexpr(const UnaryExpr* expr) {
	std::cout << "(";
	std::cout << expr->token.raw(*source);
	visit(expr->operand);
	std::cout << ")";
}

void ASTPrinter::visit_var(const VarId* var) const {
	std::cout << var->token.raw(*source);
}

void ASTPrinter::indent() {
	m_indent_level++;
}

void ASTPrinter::dedent() {
	m_indent_level--;
}

} // namespace snap