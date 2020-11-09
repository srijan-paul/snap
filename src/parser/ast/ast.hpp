#pragma once
#include "../../common.hpp"
#include "../../token.hpp"
#include <iostream>
#include <stdlib.h>
#include <string>

namespace snap {

enum class NodeType { Expr, BinExpr, Literal };

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
};

struct Literal : public Expr {
	Literal(const Token token) : Expr(NodeType::Literal, token){};
};

} // namespace snap