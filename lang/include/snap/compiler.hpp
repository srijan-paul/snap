#pragma once
#include "opcode.hpp"
#include "scanner.hpp"
#include "token.hpp"
#include "vm.hpp"
#include <array>

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
	Compiler(VM* vm, const std::string* src);
	void compile();

  private:
	VM* m_vm;
	const std::string* m_source;
	bool has_error = false;

	Scanner m_scanner;

	Token token; // current token under analysis.
	Token prev;	 // previous token.
	Token peek;	 // lookahead token.

	SymbolTable m_symtable;

	void advance(); // move 1 step forward in the token stream.

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

	/// If the next token is of type `expected` then
	/// consumes it and returns true.
	bool match(TokenType expected);
	/// If the `peek` is not of type `type` then throws
	/// an error message.
	void expect(TokenType type, const char* err_msg);

	void error_at_token(const char* message, const Token& token);
	void error_at(const char* message, u32 line);
	void error(const char* fmt...);

	void recover();

	void toplevel();
	void stmt();

	void var_decl();
	void declarator();
	void expr_stmt();

	void expr();

	void logic_or(bool can_assign);	 // ||
	void logic_and(bool can_assign); // &&

	void bit_or(bool can_assign);  // |
	void bit_and(bool can_assign); // &

	void equality(bool can_assign);	  // == !=
	void comparison(bool can_assign); // > >= < <=
	void b_shift(bool can_assign);	  // >> <<

	void sum(bool can_assign);		// + - ..
	void mult(bool can_assign);		// * / %
	void unary(bool can_assign);	// - + ! not
	void index(bool can_assign);	// [] .
	void call(bool can_assign);		// ()
	void grouping(bool can_assign); // (expr)
	void primary(bool can_assign);	// literal | id
	void literal();

	int new_variable(const Token& name);
	int find_var(const Token& name);

	inline void emit(Opcode op, u32 line);
	inline void emit(Opcode op, const Token& token);
	void emit_bytes(Opcode a, Opcode b, u32 line);
	void emit_bytes(Opcode a, Opcode b, const Token& token);
	size_t emit_value(Value value);
	size_t emit_string(const Token& token);

	Opcode toktype_to_op(TokenType type);
};

} // namespace snap