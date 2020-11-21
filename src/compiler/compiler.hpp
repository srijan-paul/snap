#pragma once
#include "../block.hpp"
#include "../opcode.hpp"
#include "../syntax/ast/ast.hpp"
#include "../vm/vm.hpp"

namespace snap {
class Compiler {
  public:
	Compiler(Block* block, const Program* ast, const std::string* src)
		: m_block{block}, m_ast{ast}, source{src} {};
	void compile();

  private:
	Block* m_block;
	const Program* m_ast;
	const std::string* source;

	void compile_stmt(const Stmt* stmt);

	void compile_vardecl(const VarDecl* decl);

	void compile_exp(const Expr* exp);
	void compile_node(const Stmt* stmt);
	void compile_binexp(const BinExpr* exp);
	void compile_literal(const Literal* exp);

	inline void emit(Opcode op);
	inline void emit(Opcode a, Opcode b);
	inline size_t emit_value(Value value);
	Opcode toktype_to_op(TokenType type);
};

} // namespace snap