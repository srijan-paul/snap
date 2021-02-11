#pragma once
#include "function.hpp"
#include "scanner.hpp"
#include "token.hpp"
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

	/// @brief Compile a top level script
	/// @return a function prototype containing the bytecode for the script.
	Prototype* compile();

	/// @brief If this compiler is compiling a function body, then
	/// reserve a stack slot for the parameter, and add the parameter
	/// name to the symbol table.
	void add_param(const Token& token);

  private:
	Prototype* m_proto;
	Compiler* m_parent = nullptr;

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

	// Compile a function's body (if this is a child compiler).
	Prototype* compile_func();

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

	void expr(bool can_assign = true);

	void logic_or(bool can_assign);	 // ||
	void logic_and(bool can_assign); // &&

	void bit_or(bool can_assign);  // |
	void bit_and(bool can_assign); // &

	void equality(bool can_assign);	  // == !=
	void comparison(bool can_assign); // > >= < <=
	void b_shift(bool can_assign);	  // >> <<

	void sum(bool can_assign);		   // + - ..
	void mult(bool can_assign);		   // * / %
	void unary(bool can_assign);	   // - + ! not
	void atomic(bool can_assign);	   // (ID|'(' EXPR ')' ) SUFFIX*
	void suffix_expr(bool can_assign); // [EXPR] | .ID | (ARGS)
	void call_args();
	void grouping(bool can_assign); // (expr)
	void primary(bool can_assign);	// literal | id
	void variable(bool can_assign);
	void literal();
	void func_expr(const String* fname);
	void table();

	/// @brief Compiles a variable assignment RHS, assumes the
	/// the very next token is an assignment or compound assign token.
	/// Note that this does not emit `set` instructions for the
	/// actual assignment, rather consumes the assignment RHS
	/// and accounts for any compound assignment operators.
	/// @param get_op The `get` opcode to use, in case it's a compound assignment.
	/// @param idx_or_name_str The index where to load the variable from, if it's a local
	///                        else the index in the constant pool for the global's name.
	void var_assign(Opcode get_op, u32 idx_or_name_str);

	/// @brief Compiles a table field assignment RHS assuming the very
	/// next token is an assignment or compound assignment token.
	/// Emits the appropriate bytecode that leaves the value on top of the 
	/// stack, but does not emit the actual 'set' opcode.
	/// @param get_op In case it's a compound assign like `+=`, we need to fetch
	/// 			 the value of that field in the table before assigning to it.
	///				 `get_op` can be one of `table_get_fast_keep` or `table_get_keep`.
	/// @param idx In case we are assigning to a table field indexed by the dot (.) operator
	/// 			 then we the `get_op` needs to use the position of that fieldn's name in the 
	///				 constant pool as an operand too.
	void table_assign(Opcode get_op, int idx);

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
	void emit(Opcode a, Opcode b);
	size_t emit_value(Value value);
	u32 emit_string(const Token& token);
	u32 emit_id_string(const Token& token);

	/// returns the corresponding bytecode
	/// from a token. e.g- TokenType::Add
	/// corresponds to Opcode::add.
	Opcode toktype_to_op(TokenType type) const;
	/// returns true if 'type' is an
	/// assignment or compound  assignment operator.
	bool is_assign_tok(TokenType ttype) const;
};

} // namespace snap