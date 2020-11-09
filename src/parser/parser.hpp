#pragma once
#include "../scanner.hpp"
#include "ast/ast.hpp"

namespace snap {

class Parser {
  public:
	Parser(const std::string* source);
	ASTNode* parse();

  private:
	const std::string* source;
	Scanner scanner;
	Token token;
	Token prev;
	Token peek;
	void advance();
	bool eof() const;
	bool check(TokenType type) const;
	bool match(TokenType type);
	bool isLiteral(TokenType type) const;

	Expr* expression();
	Expr* sum();
	Expr* mult();
	Literal* literal();
};

} // namespace snap