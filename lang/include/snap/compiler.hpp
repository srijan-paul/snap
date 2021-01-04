#pragma once
#include "block.hpp"
#include "opcode.hpp"
#include "scanner.hpp"
#include "token.hpp"
#include "value.hpp"
#include <array>


namespace snap {

struct Symbol {
	/// the distance of the local's value from the base of the VM stack
	/// at runtime.
	u8 slot = 0;

	/// whether or not this local
	/// variable has been initialized with a value.
	bool is_initialized = false;

	/// pointer to the local variable's name in the source code.
	const char* name = nullptr;
	/// length of the variable name.
	u32 length = 0;
	/// the level of nesting at which this local variable
	/// is present.
	u8 depth = 0;

	/// this field is true when the variable is
	/// initialized with a `const` qualifier. Making this
	/// Local variable immutable.
	bool is_const = false;

	Symbol(){};
	Symbol(const char* varname, u32 name_len, u8 scope_depth = 0, bool isconst = false)
		: name{varname}, length{name_len}, depth{scope_depth}, is_const{isconst} {};
};

struct SymbolTable {
	int num_symbols = 0;
	u8 scope_depth = 0;
	std::array<Symbol, UINT8_MAX + 1> m_symbols;

	int find(const char* name, int length) const;
	int find_in_current_scope(const char* name, int length) const;
	int add(const char* name, u32 length, bool is_const);
	Symbol* find_by_slot(const u8 offset);
};

class Compiler {
  public:
	VM* m_vm;
	Function* m_func = nullptr;
	static constexpr std::size_t MaxLocalVars = UINT8_MAX;
	static constexpr std::size_t MaxUpValues = UINT8_MAX;

	Compiler(VM* vm, const std::string* src);
	Function* compile();

  private:
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
		return type == TokenType::Integer || type == TokenType::String ||
			   type == TokenType::Float || type == TokenType::False || type == TokenType::True ||
			   type == TokenType::Nil;
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

	// keep eating tokens until a token
	// that may indicate the end of a block is
	// found.
	void recover();

	void toplevel();
	void stmt();

	void var_decl();
	void declarator(bool is_const);
	void block_stmt(); // {stmt*}
	void if_stmt();
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
	void variable(bool can_assign);
	void literal();

	void enter_block();
	// Exit the current scope, popping all local
	// variables off the stack and closing any upvalues.
	void exit_block();

	// Emits a jump instruction, followed by two opcodes
	// that encode the jump location. Returns the index
	// of the first half of the jump offset.
	std::size_t emit_jump(Opcode op);
	// Patches the jump instruction wjhose offset opererand is at index `index`,
	// encoding the address of the most recently emitted opcode.
	void patch_jump(std::size_t index);

	/// create a new variable and add it to the
	/// current scope in the symbol table.
	int new_variable(const Token& name, bool is_const = false);
	/// Look for a variable by it's name token, starting from
	/// the current scope, moving outward.
	/// If found, return it's stack slot.
	/// Else return -1.
	int find_var(const Token& name);

	inline void emit(Opcode op);
	inline void emit(Opcode op, u32 line);
	inline void emit(Opcode op, const Token& token);
	void emit_bytes(Opcode a, Opcode b, u32 line);
	void emit_bytes(Opcode a, Opcode b, const Token& token);
	size_t emit_value(Value value);
	size_t emit_string(const Token& token);

	/// returns the corresponding bytecode
	/// from a token. e.g- TokenType::Add
	/// corresponds to Opcode::add.
	Opcode toktype_to_op(TokenType type);
};

} // namespace snap