#pragma once
#include "../block.hpp"
#include "../syntax/ast/ast.hpp"
#include "array"

namespace snap {
enum class ExitCode { Success, CompileError, RuntimeError };

class VM {
  public:
	VM(const ASTNode* ast) : m_ast{ast} {};
	ExitCode interpret();
	~VM();

  private:
	const ASTNode* m_ast;
	const Block* m_block;
	size_t ip; // instruction ptr
	size_t sp; // stack ptr
	std::array<Value, 256> values;
};
} // namespace snap