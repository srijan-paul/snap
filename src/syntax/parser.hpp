#pragma once
#include "ast/ast.hpp"
#include "scanner.hpp"

namespace snap {

class Parser {
  public:
	Parser(const std::string* source);
	Program* parse();

  private:
	const std::string* source;
	Scanner scanner;
	Token token;
	Token prev;
	Token peek;

	void advance();

	inline bool eof() const {
		return peek.type == TokenType::Eof;
	}

	inline bool check(TokenType expected) const {
		return !eof() && peek.type == expected;
	}

	inline bool isLiteral(TokenType type) const {
		return (type == TokenType::Integer || type == TokenType::String ||
				type == TokenType::Float);
	}

	bool match(TokenType type);
	void expect(TokenType type, const char* err_msg);

	void error(const char* message);

	Program* program();
	Stmt* declaration();
	VarDecl* var_decl();
	Declarator* var_declarator();

	Stmt* stmt();

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