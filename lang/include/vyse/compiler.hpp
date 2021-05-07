#pragma once
#include "common.hpp"
#include "function.hpp"
#include "opcode.hpp"
#include "scanner.hpp"
#include <array>

namespace vyse {

struct LocalVar {
	/// Pointer to the local variable's name in the source code.
	/// This points to some place inside the source string and therefore
	/// accessing this field only makes sense as long as the source
	/// string has not been freed.
	const char* name = nullptr;
	/// Length of the variable name.
	u32 length = 0;
	/// The level of nesting at which this local variable
	/// is present. Needed at compile time only.
	u8 depth = 0;

	/// This field is true when the variable is
	/// initialized with a `const` qualifier. Making this
	/// Local variable immutable. Needed at compile time
	/// only.
	bool is_const = false;

	// `true` if this local variable has been captured by nested closure
	// as an upvalue. Needed at compile time only.
	bool is_captured = false;

	explicit LocalVar() noexcept {};
	explicit LocalVar(const char* varname, u32 name_len, u8 scope_depth = 0,
										bool isconst = false) noexcept
			: name{varname}, length{name_len}, depth{scope_depth}, is_const{isconst} {};
};

struct UpvalDesc {
	int index = -1;
	bool is_const = false;
	bool is_local = false;
};

struct SymbolTable {
	u32 m_scope_depth = 0;
	int m_num_symbols = 0;
	int m_num_upvals = 0;
	std::array<LocalVar, UINT8_MAX + 1> m_symbols;
	std::array<UpvalDesc, UINT8_MAX + 1> m_upvals;

	// Recursively search the nested scopes going outward,
	// looking for a variable with the name `name`.
	int find(const char* name, int length) const;
	int find_in_current_scope(const char* name, int length) const;
	int add(const char* name, u32 length, bool is_const);

	// add an upvalue to the `m_upvals` array, if it exists already
	// then don't add a copy, instead return the index.
	int add_upvalue(int index, bool is_local, bool is_const);
	// takes in the index of a local variable and returns it's Symbol info.
	const LocalVar* find_by_slot(const u8 offset) const;
};

enum class ExpKind {
	literal_value,
	identifier,
	prefix,
	call,
	index,
	member_access,
	none,
};

class Compiler {
	friend GC;

public:
	static constexpr u8 MaxLocalVars = UINT8_MAX;
	static constexpr u8 MaxUpValues = UINT8_MAX;
	static constexpr u8 MaxConstants = UINT8_MAX;
	static constexpr u8 MaxFuncParams = 200;

	VYSE_NO_COPY(Compiler);
	VYSE_NO_MOVE(Compiler);
	VYSE_NO_DEFAULT_CONSTRUCT(Compiler);

	// Creates a fresh new compiler that will
	// parse the toplevel script `src`.
	explicit Compiler(VM* vm, const std::string* src) noexcept;

	/// @brief Create a child compiler used for
	/// compiling function bodies. This assumes
	/// the parent compiler has already consumed
	/// the everything up to the functions body.
	/// @param parent The parent compiler that created this compiler. Used for looking up upvalues.
	/// @param fname name of the function that this compiler is commpiling into.
	explicit Compiler(VM* vm, Compiler* parent, String* fname) noexcept;

	~Compiler();

	/// @brief Compile a top level script
	/// @return a function's codeblock containing the bytecode for the script.
	[[nodiscard]] CodeBlock* compile();

	/// @brief returns true if the compiler
	/// has encountered an error while compiling
	/// the source.
	bool ok() const noexcept;

	/// Provided a bytecode instruction's index, returns the number of
	/// operands it takes.
	int op_arity(u32 indx) const noexcept;

	/// Returns [op]'s stack effect. The stack effect of an
	/// opcode is the number of values it pushes/pops on/off the
	/// stack. A stack effect of -1 means the opcode pops one item
	/// off the stack.
	int op_stack_effect(Opcode op) const noexcept;

private:
	struct Loop {
		VYSE_NO_DEFAULT_CONSTRUCT(Loop);
		enum class Type {
			For,
			While,
		};
		Loop(Type type) : loop_type(type){};

		Type loop_type;
		Loop* enclosing;
		/// The first instruction of this loop.
		u32 start;
		/// The scope depth of the loop's statement
		/// (for, while etc) itself. The contents of
		/// the loop's body (if it's a block)
		/// have depth greater by 1.
		u32 scope_depth;
	};

	VM* m_vm;
	CodeBlock* m_codeblock;
	Compiler* const m_parent = nullptr;
	Loop* m_loop = nullptr;

	/// Running sum of the stack size.
	/// This is calculated by adding together
	/// the stack effects of emitted instructions.
	s64 m_stack_size = 0;

	const std::string* m_source;
	bool has_error = false;
	// When true, the compiler goes into
	// error recovery mode, trying to eat all
	// tokens until some that might denote the
	// beginning of a statement is encountered.
	bool panic = false;
	/// The scanner object that this compiler
	/// draws tokens from. This is a pointer
	/// because the nested child compilers will
	/// want to draw tokens from the same scanner.
	Scanner* m_scanner;

	Token token; // current token under analysis.
	Token prev;	 // previous token.
	Token peek;	 // lookahead token.

	SymbolTable m_symtable;

	void advance(); // move 1 step forward in the token stream.

	inline bool eof() const noexcept {
		return peek.type == TokenType::Eof;
	}

	inline bool check(TokenType expected) const noexcept {
		return !eof() && peek.type == expected;
	}

	inline bool is_literal(TokenType type) const noexcept {
		return type == TokenType::Integer || type == TokenType::String || type == TokenType::Float ||
					 type == TokenType::False || type == TokenType::True || type == TokenType::Nil;
	}

	/// If the next token is of type `expected` then
	/// consumes it and returns true.
	bool match(TokenType expected) noexcept;
	/// If the `peek` is not of type `type` then throws
	/// an error message. Else consumes the token and stays quiet.
	void expect(TokenType type, const char* err_msg);

	/// If `peek` is not of the type `type` then throws
	/// an error message.
	void test(TokenType type, const char* errmsg);

	void error_at_token(const char* message, const Token& token);
	void error_at(const char* message, u32 line);
	void error(std::string&& message);

	// Compile a function's body (if this is a child compiler).
	CodeBlock* compile_func();

	// keep eating tokens until a token
	// that may indicate the end of a block or statement
	// is found.
	void recover();

	void toplevel();

	void var_decl();								// (let|const) DECL+
	void declarator(bool is_const); // ID (= EXPR)?
	void block_stmt();							// {stmt*}
	void if_stmt();									// if EXPR STMT (else STMT)?
	void while_stmt();							// while EXPR STMT
	void for_stmt();								// for ID = EXP, EXP (, EXP)? STMT
	void break_stmt();							// BREAK
	void continue_stmt();						// CONTINUE
	void fn_decl();									// fn (ID|SUFFIXED_EXPR) BLOCK
	void ret_stmt();								// return EXPR?
	void expr_stmt();								// FUNCALL | ASSIGN
	void complete_expr_stmt(ExpKind prefix_type);

	// parses a expression statement's prefix.
	// returns true if the statement was a
	// complete expression statement by itself
	// (such as 'a = 1') else returns false.
	ExpKind prefix();

	void expr();

	void logic_or();	// || or
	void logic_and(); // && and

	void bit_or();	// |
	void bit_and(); // &

	void equality();	 // == !=
	void comparison(); // > >= < <=
	void b_shift();		 // >> <<

	void sum();					// + - ..
	void mult();				// * / %
	void unary();				// - + ! not
	void atomic();			// (ID|'(' EXPR ')' ) SUFFIX*
	void suffix_expr(); // '['EXPR']' | '.'ID | '('ARGS')' | :ID'('')'

	/// @brief Compiles the arguments for a call expression. The current token
	/// must be the opening '(' for the argument
	void compile_args(bool is_method = false);						 // EXPR (',' EXPR)*
	void grouping();																			 // '('expr')'
	void primary();																				 // LITERAL | ID
	void variable(bool can_assign);												 // ID
	void literal();																				 // NUM | STR | BOOL | nil
	void func_expr(String* fname, bool is_method = false); // fn NAME? BLOCK

	/// @brief compiles a table, assuming the opening '{' has been
	/// consumed.
	void table();

	/// @brief Compiles a variable assignment RHS, assumes the
	/// the very next token is an assignment or compound assign token.
	/// Note that this does not emit `set` instructions for the
	/// actual assignment, rather consumes the assignment RHS
	/// and accounts for any compound assignment operators.
	/// @param get_op The 'get' opcode to use, in case it's a compound assignment.
	/// @param idx_or_name_str The index where to load the variable from, if it's a local
	///                        else the index in the constant pool for the global's name.
	void var_assign(Opcode get_op, u32 idx_or_name_str);

	/// @brief Compiles a table field assignment RHS assuming the very
	/// next token is an assignment or compound assignment token.
	/// Emits the appropriate bytecode that leaves the value on top of the
	/// stack, but does not emit the actual 'set' opcode.
	/// @param get_op In case it's a compound assign like `+=`, we need to fetch
	/// 			 the value of that field in the table before assigning to it.
	///				 [get_op] can be one of `index_no_pop` or `table_get_no_pop`.
	/// @param idx In case we are assigning to a table field indexed by the dot (.) operator
	/// 			 then we the [get_op] needs to use the position of that field's name in the
	///				 constant pool as an operand too.
	void table_assign(Opcode get_op, int idx);

	void enter_block() noexcept;

	/// Emit pop and close instructions for the variables and upvalues
	/// upto scope depth [depth]. Note that this only emits instructions
	/// to discard the locals at runtime, but doesn't actually remove
	/// them from the symbol table. This is used to emit instructions for
	/// 'break' and 'continue' statements.
	void discard_loop_locals(u32 depth);
	// Exit the current scope, emitting pop instructions
	// for all local variables and close instructions for upvalues.
	void exit_block();

	void enter_loop(Loop& loop);

	/// Exits the innermost loop that we are inside of and
	/// uses [jump_instr] to emit a jump from the end of the loop back
	/// to the first instruction in the loop's boddy by backpatching it.
	void exit_loop(Opcode jump_instr);

	// Emits a jump instruction, followed by two opcodes
	// that encode the jump location in big endian.
	// Returns the index  of the first half of the jump offset.
	size_t emit_jump(Opcode op);
	// Patches the jump instruction whose first operand is at index [index],
	// encoding the address of the most recently emitted opcode.
	void patch_jump(size_t index);
	// Patches a jump instruction whose first operand is at index [index]
	// that's supposed  to  jump backwards to the opcode at at index [dst_index]
	void patch_backwards_jump(size_t index, u32 dst_index);

	/// @brief create a new variable and add it to the
	/// current scope in the symbol table.
	int new_variable(const Token& name, bool is_const = false);
	int new_variable(const char* name, u32 len, bool is_const = false);

	/// @brief If this compiler is compiling a function body, then
	/// reserve a stack slot for the parameter, and add the parameter
	/// name to the symbol table.
	void add_param(const Token& token);

	/// @brief Adds the 'self' parameter to the list of
	/// locals, reserving a stack slot for it at runtime.
	void add_self_param();

	/// Look for a variable by it's name token, starting from
	/// the current scope, moving outward.
	/// If found, return it's stack slot.
	/// Else return -1.
	int find_local_var(const Token& name) const noexcept;

	/// Find an upvalue by it's name token.
	/// Starts looking in the current symboltable's upvalue
	/// array, If it isn't found there then it recursively
	/// searches up the parent compiler chain repeating the process.
	/// Once an upvalue is found, it adds it the current
	/// Upvalue list and returns an index to that.
	/// If no upvalue is found, returns -1.
	int find_upvalue(const Token& name);

	inline void emit(Opcode op);
	inline void emit(Opcode a, Opcode b);
	inline void emit(Opcode op, const Token& token);
	inline void emit_arg(u8 arg);
	inline void emit_with_arg(Opcode opm, u8 arg);

	size_t emit_value(Value value);
	u32 emit_string(const Token& token);
	u32 emit_id_string(const Token& token);

	/// @brief returns the corresponding bytecode
	/// from a token. e.g- TokenType::Add
	/// corresponds to Opcode::add.
	Opcode toktype_to_op(TokenType type) const noexcept;

	/// @brief returns true if 'type' is an
	/// assignment or compound  assignment operator.
	bool is_assign_tok(TokenType ttype) const noexcept;
};

} // namespace vyse