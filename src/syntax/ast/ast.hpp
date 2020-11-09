#pragma once
#include "../../common.hpp"
#include "../../token.hpp"
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

namespace snap {

enum class NodeType { Expr, BinExpr, UnaryExpr, Literal, VarDeclarator, VarDeclaration };

using ASTVisitor = class ASTVisitor;

class ASTNode {
  public:
	NodeType type;
	ASTNode(NodeType type) : type{type} {};
};

struct Expr : public ASTNode {
	const Token token;
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

struct Declarator : Stmt {
	Token var;
	Expr* init{nullptr};
	Declarator(Token tk) : Stmt(NodeType::VarDeclaration), var{tk} {};
};

struct VarDecl : Stmt {
	std::vector<Declarator> declarations = {};
	VarDecl() : Stmt(NodeType::VarDeclarator){};
};

} // namespace snap