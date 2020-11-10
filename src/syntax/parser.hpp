#pragma once
#include "ast/ast.hpp"
#include "scanner.hpp"

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
	Expr* assign();

	Expr* logic_or();
	Expr* logic_and();

	Expr* bit_or(); // TODO: Bit XOR
	Expr* bit_and();

	Expr* equality();
	Expr* comparison();

	Expr* bit_shift();

	Expr* sum();
	Expr* mult();
	Expr* unary();
	Literal* literal();
};

} // namespace snap