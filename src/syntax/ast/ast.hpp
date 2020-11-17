#pragma once
#include "../../common.hpp"
#include "../../token.hpp"
#include "../../typecheck/type.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace snap {

enum class NodeType {
	Expr,
	BinExpr,
	UnaryExpr,
	Literal,
	VarDeclarator,
	VarDeclaration,
	ExprStmt,
	Program
};

class ASTNode {
  public:
	NodeType type;
	ASTNode(NodeType type) : type{type} {};
};

struct Expr : public ASTNode {
	const Token token;
	const Type* data_type{nullptr};
	Expr(NodeType type, const Token op) : ASTNode(type), token{op} {};
};

struct BinExpr : public Expr {
	Expr* left;
	Expr* right;
	BinExpr(Expr* left, const Token op, Expr* right)
		: left{left}, right{right}, Expr(NodeType::BinExpr, op){};

	~BinExpr() {
		delete left;
		delete right;
	}
};

struct UnaryExpr : Expr {
	Expr* operand;
	UnaryExpr(Token op, Expr* exp) : Expr(NodeType::UnaryExpr, op), operand(exp){};
};

struct Literal : Expr {
	Literal(const Token token) : Expr(NodeType::Literal, token){};
};

struct Stmt : public ASTNode {
	Location location;
	Stmt(NodeType type) : ASTNode(type){};
};

struct ExprStmt : Stmt {
	Expr* exp;
	ExprStmt(Expr* expr) : Stmt(NodeType::ExprStmt), exp{expr} {};
};

struct Declaration : Stmt {
	Declaration(NodeType type) : Stmt(type){};
};

struct Declarator : Stmt {
	Token var;
	Expr* init{nullptr};
	Declarator(Token tk) : Stmt(NodeType::VarDeclaration), var{tk} {};
};

struct VarDecl : Declaration {
	std::vector<Declarator> declarations = {};
	VarDecl() : Declaration(NodeType::VarDeclarator){};
};

struct Program : public ASTNode {
	std::vector<Stmt*> stmts;
	Program() : ASTNode(NodeType::Program){};
};

} // namespace snap