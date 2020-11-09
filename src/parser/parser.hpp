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

	Literal* literal();
};

} // namespace snap