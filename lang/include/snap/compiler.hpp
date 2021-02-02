#pragma once
#include "block.hpp"
#include "opcode.hpp"
#include "scanner.hpp"
#include "token.hpp"
#include "value.hpp"
#include <array>

namespace snap {

struct Symbol {
	/// the distance of the local's value from the base of it's function's stack
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

	// true if this local variable has been captured by nested closure
	// as an upvalue.
	bool is_captured = false;

	Symbol(){};
	Symbol(const char* varname, u32 name_len, u8 scope_depth = 0, bool isconst = false)
		: name{varname}, length{name_len}, depth{scope_depth}, is_const{isconst} {};
};

struct CompilerUpval {
	int index = -1;
	bool is_const = false;
	bool is_local = false;
};

struct SymbolTable {
	int num_symbols = 0;
	u32 m_scope_depth = 0;
	std::array<Symbol, UINT8_MAX + 1> m_symbols;

	int num_upvals = 0;
	std::array<CompilerUpval, UINT8_MAX + 1> m_upvals;

	// Recursively search the nested scopes going outward,
	// looking for a variable with the name `name`.
	int find(const char* name, int length) const;
	int find_in_current_scope(const char* name, int length) const;
	int add(const char* name, u32 length, bool is_const);

	// add an upvalue to the `m_upvals` array, if it exists already
	// then don't add a copy, instead return the index.
	int add_upvalue(u8 index, bool is_local, bool is_const);
	// takes in the index of a local variable and returns it's Symbol info.
	Symbol* find_by_slot(const u8 offset);
};

class Compiler {
  public:
	VM* m_vm;
	Prototype* m_proto;
	Compiler* m_parent = nullptr;

	static constexpr u8 MaxLocalVars = UINT8_MAX;
	static constexpr u8 MaxUpValues = UINT8_MAX;
	static constexpr u8 MaxFuncParams = UINT8_MAX;

	// Creates a fresh new compiler that will
	// parse the toplevel script `src`.
	Compiler(VM* vm, const std::string* src);

	/// @brief Create a child compiler used for
	/// compiling function bodies. This assumes
	/// the parent compiler has already consumed
	/// the everything up to the functions body.
	/// @param parent The parent compiler that created this compiler. Used for looking up upvalues.
	/// @param fname name of the function that this compiler is commpiling into.
	Compiler(VM* vm, Compiler* parent, const String* fname);

	~Compiler();

	// Compile a top level script
	Prototype* compile();

	// Compile a function's body (if this is a child compiler).
	Prototype* compile_func();

	/// @brief If this compiler is compiling a function body, then
	/// reserve a stack slot for the parameter, and add the parameter
	/// name to the symbol table.
	void add_param(const Token& token);

  private:
	const std::string* m_source;
	bool has_error = false;
	Scanner* m_scanner;

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
	/// an error message. Else consumes the token and stays quiet.
	void expect(TokenType type, const char* err_msg);

	/// If `peek` is not of the type `type` then throws
	/// an error message.
	void test(TokenType type, const char* errmsg);

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
	void fn_decl();
	void ret_stmt();

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
	void func_expr(const String* fname);

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
	int find_local_var(const Token& name);

	// Find an upvalue by it's name token.
	// Starts looking in the current symboltable's upvalue
	// array, If it isn't found there then it recursively
	// searches up the parent compiler chain repeating the process.
	// Once an upvalue is found, it adds it the current
	// Upvalue list and returns an index to that.
	// If no upvalue is found, returns -1.
	int find_upvalue(const Token& name);

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