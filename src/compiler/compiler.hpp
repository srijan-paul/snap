#pragma once
#include "../block.hpp"
#include "../opcode.hpp"
#include "../syntax/ast/ast.hpp"
#include "../vm/vm.hpp"
#include <array>
#include <stdint.h>

namespace snap {

struct Symbol {
	/// the distance of the local's value from the base of the VM stack
	/// at runtime.
	u8 slot = 0;
	/// the level of nesting at which this local variable
	/// is present.
	u8 depth = 0;
	/// whether or not this local
	/// variable has been initialized with a value.
	bool is_initialized = false;

	/// pointer to the local variable's name in the source code.
	const char* name = nullptr;
	/// length of the variable name.
	u32 length = 0;

	/// this field is true when the variable is
	/// initialized with a `const` qualifier. Making this
	/// Local variable immutable.
	bool is_const = false;

	Symbol(){};
	Symbol(const char* varname, u32 name_len, u8 scope_depth = 0)
		: name{varname}, length{name_len}, depth{scope_depth} {};
};

struct SymbolTable {
	int num_symbols = 0;
	u8 scope_depth = 0;
	int find(const char* name, int length) const;
	int find_in_current_scope(const char* name, int length) const;
	int add(const char* name, u32 length);
	int find_by_slot(const u8 offset);

  private:
	std::array<Symbol, UINT8_MAX + 1> symbols;
};

class Compiler {
  public:
	Compiler(Block* block, const Program* ast, const std::string* src)
		: m_block{block}, m_ast{ast}, source{src} {};
	void compile();

  private:
	Block* m_block;
	const Program* m_ast;
	const std::string* source;

	SymbolTable symbol_table;

	void compile_stmt(const Stmt* stmt);

	void compile_vardecl(const VarDecl* decl);

	void compile_exp(const Expr* exp);
	void compile_node(const Stmt* stmt);
	void compile_binexp(const BinExpr* exp);
	void compile_literal(const Literal* exp);
	void compile_var(const VarId* var);

	int new_variable(const Token* name);

	inline void emit(Opcode op);
	inline void emit(Opcode a, Opcode b);
	inline size_t emit_value(Value value);
	Opcode toktype_to_op(TokenType type);
};

} // namespace snap