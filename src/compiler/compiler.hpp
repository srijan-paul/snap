#pragma once
#include "../block.hpp"
#include "../syntax/ast/ast.hpp"
#include "../vm/vm.hpp"

namespace snap {
class Compiler {
  public:
	Compiler(const Block* block, const ASTNode* ast) : m_block{block}, m_ast{ast} {};
	void compile();

  private:
	const Block* m_block;
	const ASTNode* m_ast;
};

} // namespace snap