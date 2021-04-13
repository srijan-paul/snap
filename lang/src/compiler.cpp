#include "debug.hpp"
#include "str_format.hpp"
#include <compiler.hpp>
#include <cstdio>
#include <cstring>
#include <string>
#include <vm.hpp>

#define TOK2NUM(t) SNAP_NUM_VAL(std::stod(t.raw(*m_source)))
#define THIS_BLOCK (m_codeblock->block())
#define ERROR(...) (error(kt::format_str(__VA_ARGS__)))

#define DEFINE_PARSE_FN(name, cond, next_fn)                                                       \
	void name(bool can_assign) {                                                                     \
		next_fn(can_assign);                                                                           \
		while (cond) {                                                                                 \
			const Token op_token = token;                                                                \
			next_fn(false);                                                                              \
			emit(toktype_to_op(op_token.type), op_token);                                                \
		}                                                                                              \
	}

namespace snap {

using Op = Opcode;
using TT = TokenType;

Compiler::Compiler(VM* vm, const std::string* src) noexcept : m_vm{vm}, m_source{src} {
	m_scanner = new Scanner{src};
	advance(); // set `peek` to the first token in the token stream.
	String* fname = &vm->string("<script>", 8);
	// reserve the first slot for this toplevel function.
	m_symtable.add("<script>", 8, true);

	// When allocating [m_codeblock], the String "script"
	// not reachable by the VM, so we protect it from GC.
	m_vm->gc_protect(fname);
	m_codeblock = &vm->make<CodeBlock>(fname);

	// Now the string is reachable through the code
	// block, So we can unprotect it.
	m_vm->gc_unprotect(fname);
}

Compiler::Compiler(VM* vm, Compiler* parent, String* name) noexcept : m_vm{vm}, m_parent{parent} {
	m_scanner = m_parent->m_scanner;
	m_codeblock = &m_vm->make<CodeBlock>(name);

	m_symtable.add(name->c_str(), name->len(), false);

	m_source = parent->m_source;
	vm->m_compiler = this;

	prev = parent->prev;
	token = parent->token;
	peek = parent->peek;
}

Compiler::~Compiler() {
	// If this is the top-level compiler then we can
	// delete the scanner assosciated with it.
	// TODO: use std::shared_ptr<> to auotmate this
	// deallocation and get rid of the destructor.
	if (m_parent == nullptr) {
		delete m_scanner;
	}
}

CodeBlock* Compiler::compile() {
	while (!eof()) {
		toplevel();
	}

	emit(Op::load_nil, Op::return_val);
	m_codeblock->m_num_upvals = m_symtable.m_num_upvals;
	return m_codeblock;
}

CodeBlock* Compiler::compile_func() {
	test(TT::LCurlBrace, "Expected '{' before function body.");

	block_stmt();
	emit(Op::load_nil, Op::return_val);
	m_codeblock->m_num_upvals = m_symtable.m_num_upvals;
	m_vm->m_compiler = m_parent;
	return m_codeblock;
}

// top level statements are one of:
// - var declaration
// - function declaration
// - expression statement
// - export statement
void Compiler::toplevel() {
	if (panic) recover();
	if (eof()) return;

	const TT tt = peek.type;

	switch (tt) {
	case TT::Const:
	case TT::Let: return var_decl();
	case TT::LCurlBrace: return block_stmt();
	case TT::If: return if_stmt();
	case TT::Fn: return fn_decl();
	case TT::Return: return ret_stmt();
	default: expr_stmt();
	}
}

void Compiler::var_decl() {
	advance(); // eat the 'let'|'const'
	const bool is_const = token.type == TT::Const;
	do {
		declarator(is_const);
	} while (match(TT::Comma));
	match(TT::Semi); // optional semi colon at the end.
}

void Compiler::declarator(bool is_const) {
	expect(TT::Id, "Expected variable name.");
	// add the new variable to the symbol table.
	// default value is `nil`.
	const Token name = token;

	match(TT::Eq) ? expr() : emit(Op::load_nil, token.location.line);
	new_variable(name, is_const);
}

void Compiler::block_stmt() {
	advance(); // eat the opening '{'
	enter_block();
	while (!(eof() or check(TT::RCurlBrace))) {
		toplevel();
	}
	expect(TT::RCurlBrace, "Expected '}' to close block.");
	exit_block();
}

void Compiler::if_stmt() {
	advance(); // consume 'if'
	expr();		 // parse condition.
	const std::size_t jmp = emit_jump(Op::pop_jmp_if_false);
	toplevel();

	if (match(TT::Else)) {
		std::size_t else_jmp = emit_jump(Op::jmp);
		patch_jump(jmp);
		toplevel();
		patch_jump(else_jmp);
		return;
	}

	patch_jump(jmp);
}

void Compiler::fn_decl() {
	advance(); // consume 'fn' token.
	expect(TT::Id, "expected function name");

	const Token name_token = token;
	String* fname = &m_vm->string(name_token.raw_cstr(m_source), name_token.length());

	func_expr(fname);

	new_variable(name_token);
}

/// Compiles the body of a function or method assuming everything until the
/// the opening parenthesis has been consumed.
void Compiler::func_expr(String* fname, bool is_method) {
	// When compiling the body of the function, we allocate
	// a code block; at that point in time, the name of the
	// function is not reachable by the Garbage Collector,
	// so we protect it.
	m_vm->gc_protect(fname);
	Compiler compiler{m_vm, this, fname};

	compiler.expect(TT::LParen, "Expected '(' before function parameters.");

	int param_count = 0;

	if (is_method) {
		++param_count;
		compiler.add_self_param();
	}

	if (!compiler.check(TT::RParen)) {
		do {
			compiler.expect(TT::Id, "Expected parameter name.");
			compiler.add_param(compiler.token);
			++param_count;
		} while (compiler.match(TT::Comma));
	}

	if (param_count > MaxFuncParams) {
		compiler.error_at_token("Function cannot have more than 200 parameters", compiler.token);
	}

	compiler.expect(TT::RParen, "Expected ')' after function parameters.");

	CodeBlock* code = compiler.compile_func();
	const u8 idx = emit_value(SNAP_OBJECT_VAL(code));

	emit_bytes(Op::make_func, static_cast<Op>(idx), token);
	emit(static_cast<Op>(code->m_num_upvals), token);

	// Now, [fname] can be reached via the code block itself.
	m_vm->gc_unprotect(fname);

	for (int i = 0; i < compiler.m_symtable.m_num_upvals; ++i) {
		const UpvalDesc& upval = compiler.m_symtable.m_upvals[i];
		emit(upval.is_local ? static_cast<Op>(1) : static_cast<Op>(0));
		emit(static_cast<Op>(upval.index));
	}

#ifdef SNAP_DEBUG_DISASSEMBLY
	disassemble_block(code->name_cstr(), code->block());
#endif

	prev = compiler.prev;
	token = compiler.token;
	peek = compiler.peek;
}

void Compiler::ret_stmt() {
	advance(); // eat the return keyword.
	// If the next token marks the start of an expression, then
	// compile this statement as `return EXPR`, else it's just a `return`.
	// where a `nil` after the return is implicit.
	if (isLiteral(peek.type) or check(TT::Id) or check(TT::Bang) or check(TT::Minus) or
			check(TT::LParen) or check(TT::Fn) or check(TT::LCurlBrace)) {
		expr();
	} else {
		emit(Op::load_nil);
	}
	emit(Opcode::return_val);
	match(TT::Semi);
}

void Compiler::expr_stmt() {
	expr();
	emit(Opcode::pop, token);
	match(TT::Semi);
}

void Compiler::expr(bool can_assign) {
	logic_or(can_assign);
}

void Compiler::logic_or(bool can_assign) {
	logic_and(can_assign);
	if (match(TT::Or)) {
		const std::size_t jump = emit_jump(Op::jmp_if_true_or_pop);
		logic_or(false);
		patch_jump(jump);
	}
}

void Compiler::logic_and(bool can_assign) {
	bit_or(can_assign);
	if (match(TT::And)) {
		std::size_t jump = emit_jump(Op::jmp_if_false_or_pop);
		logic_and(false);
		patch_jump(jump);
	}
}

DEFINE_PARSE_FN(Compiler::bit_or, match(TT::BitOr), bit_and)
DEFINE_PARSE_FN(Compiler::bit_and, match(TT::BitAnd), equality)
DEFINE_PARSE_FN(Compiler::equality, match(TT::EqEq) or match(TT::BangEq), comparison)
DEFINE_PARSE_FN(Compiler::comparison,
								match(TT::Gt) or match(TT::Lt) or match(TT::GtEq) or match(TT::LtEq), b_shift)
DEFINE_PARSE_FN(Compiler::b_shift, match(TT::BitLShift) or match(TT::BitRShift), sum)
DEFINE_PARSE_FN(Compiler::sum, (match(TT::Plus) or match(TT::Minus) or match(TT::Concat)), mult)
DEFINE_PARSE_FN(Compiler::mult, (match(TT::Mult) or match(TT::Mod) or match(TT::Div)), unary)

void Compiler::unary(bool can_assign) {
	if (check(TT::Bang) or check(TT::Minus)) {
		advance();
		const Token op_token = token;
		atomic(false);
		switch (op_token.type) {
		case TT::Bang: emit(Op::lnot, op_token); break;
		case TT::Minus: emit(Op::negate, op_token); break;
		default: SNAP_ERROR("Impossible token.");
		}
		return;
	}
	atomic(can_assign);
}

void Compiler::atomic(bool can_assign) {
	grouping(can_assign);
	suffix_expr(can_assign);
}

void Compiler::suffix_expr(bool can_assign) {
	while (true) {
		switch (peek.type) {
		case TT::LSqBrace: {
			advance();
			expr();
			expect(TT::RSqBrace, "Expected ']' to close index expression.");
			if (can_assign and is_assign_tok(peek.type)) {
				table_assign(Op::index_no_pop, -1);
				emit(Op::index_set);
				return;
			} else {
				emit(Op::index);
			}
			break;
		}
		case TT::LParen: compile_args(); break;
		case TT::Dot: {
			advance();
			expect(TT::Id, "Expected field name.");
			const u8 index = emit_id_string(token);

			if (can_assign and is_assign_tok(peek.type)) {
				table_assign(Op::table_get_no_pop, index);
				emit(Op::table_set, Op(index));
				return;
			} else {
				emit(Op::table_get, static_cast<Op>(index));
			}
			break;
		}
		case TT::Colon: {
			advance();
			expect(TT::Id, "Expected method name.");
			u8 index = emit_id_string(token);
			emit(Op::prep_method_call, Op(index));
			compile_args(true);
			break;
		}
		default: return;
		}
	}
}

void Compiler::table_assign(Op get_op, int idx) {
	advance();
	const TT ttype = token.type;

	if (ttype == TT::Eq) {
		expr();
		return;
	}

	emit(get_op);
	if (idx > 0) emit(Op(idx));
	expr();
	emit(toktype_to_op(ttype));
}

void Compiler::compile_args(bool is_method) {
	advance(); // eat opening '('

	/// If it's a method call, then start with 1
	/// argument count for the implicit 'self' argument.
	u32 argc = is_method ? 1 : 0;

	if (!check(TT::RParen)) {
		do {
			++argc;
			if (argc > MaxFuncParams) ERROR("Too many arguments to function call.");
			expr(); // push the arguments on the stack,
		} while (match(TT::Comma));
	}

	expect(TT::RParen, "Expected ')' after call.");
	emit(Op::call_func, Op(argc));
}

void Compiler::grouping(bool can_assign) {
	if (match(TT::LParen)) {
		expr();
		expect(TT::RParen, "Expected ')' after grouping expression.");
		return;
	}
	primary(can_assign);
}

void Compiler::primary(bool can_assign) {
	if (isLiteral(peek.type)) {
		literal();
	} else if (match(TT::Fn)) {
		static constexpr const char* name = "<anonymous>";
		String* fname = &m_vm->string(name, 11);
		if (check(TT::LParen)) return func_expr(fname);
		// Names of lambda expressions are simply ignored
		// Unless found in a statement context.
		expect(TT::Id, "Expected function name or '('.");
		return func_expr(fname);
	} else if (check(TT::Id)) {
		variable(can_assign);
	} else if (match(TT::LCurlBrace)) {
		table();
	} else {
		error_at(kt::format_str("Unexpected '{}'.", peek.raw(*m_source)).c_str(), peek.location.line);
		advance();
	}
}

/// @brief compiles a table, assuming the opening '{' has been
/// consumed.
void Compiler::table() {
	emit(Opcode::new_table);

	if (match(TT::RCurlBrace)) return;

	do {
		if (match(TT::LSqBrace)) {
			expr();
			expect(TT::RSqBrace, "Expected ']' near table key.");
		} else {
			expect(TT::Id, "Expected identifier as table key.");
			String* key_string = &m_vm->string(token.raw(*m_source).c_str(), token.length());
			const int key_idx = emit_value(SNAP_OBJECT_VAL(key_string));
			emit_bytes(Op::load_const, static_cast<Op>(key_idx), token);
			if (check(TT::LParen)) {
				func_expr(key_string, true);
				emit(Op::table_add_field);
				if (check(TT::RCurlBrace)) break;
				continue;
			}
		}

		expect(TT::Colon, "Expected ':' after table key.");
		expr(false);
		emit(Op::table_add_field);

		if (check(TT::RCurlBrace)) break;
	} while (!eof() and match(TT::Comma));

	if (eof()) {
		error("Reached end of file while compiling.");
		return;
	}

	expect(TT::RCurlBrace, "Expected '}' to close table or ',' to separate entry.");
}

void Compiler::variable(bool can_assign) {
	advance();

	Op get_op = Op::get_var;
	Op set_op = Op::set_var;

	int index = find_local_var(token);
	bool is_const = (index == -1) ? false : m_symtable.find_by_slot(index)->is_const;

	// if no local variable with that name was found then look for an
	// upvalue.
	if (index == -1) {
		index = find_upvalue(token);

		if (index == -1) {
			get_op = Opcode::get_global;
			set_op = Opcode::set_global;
			index = emit_id_string(token);
		} else {
			get_op = Opcode::get_upval;
			set_op = Opcode::set_upval;
			is_const = m_symtable.m_upvals[index].is_const;
		}
	}

	if (can_assign and is_assign_tok(peek.type)) {
		if (is_const) {
			std::string message =
					kt::format_str("Cannot assign to variable '{}' marked const.", token.raw(*m_source));
			error_at_token(message.c_str(), token);
			panic = false; // Don't send the compiler into error recovery mode.
		}

		/// Compile the RHS of the assignment, and
		/// any necessary arithmetic ops if its a
		/// compound assignment operator. So by the
		/// time we are setting the value, the RHS
		/// is sitting ready on top of the stack.
		var_assign(get_op, index);
		emit_bytes(set_op, static_cast<Op>(index), token);
	} else {
		emit_bytes(get_op, static_cast<Op>(index), token);
	}
}

void Compiler::var_assign(Op get_op, u32 idx_or_name_str) {
	advance();
	const TT ttype = token.type;
	if (ttype == TT::Eq) {
		expr();
		return;
	}

	emit(get_op, Op(idx_or_name_str));
	expr();
	emit(toktype_to_op(ttype));
}

void Compiler::literal() {
	advance();
	std::size_t index = 0;
	switch (token.type) {
	case TT::Integer:
	case TT::Float: index = emit_value(TOK2NUM(token)); break;
	case TT::String: index = emit_string(token); break;
	case TT::True: index = emit_value(SNAP_BOOL_VAL(true)); break;
	case TT::False: index = emit_value(SNAP_BOOL_VAL(false)); break;
	case TT::Nil: index = emit_value(SNAP_NIL_VAL); break;
	default: SNAP_ERROR("Impossible code reached.");
	}

	emit(Op::load_const, static_cast<Op>(index));
}

void Compiler::recover() {
	while (!eof()) {
		advance();
		if (token.type == TT::Semi or token.type == TT::RCurlBrace or token.type == TT::RSqBrace) {
			advance();
			break;
		}
	}
	panic = false;
}

void Compiler::enter_block() {
	++m_symtable.m_scope_depth;
}

void Compiler::exit_block() {
	for (int i = m_symtable.m_num_symbols - 1; i >= 0; i--) {
		const LocalVar& var = m_symtable.m_symbols[i];

		if (var.depth != m_symtable.m_scope_depth) break;

		emit(var.is_captured ? Op::close_upval : Op::pop);
		--m_symtable.m_num_symbols;
	}

	--m_symtable.m_scope_depth;
}

std::size_t Compiler::emit_jump(Opcode op) {
	std::size_t index = THIS_BLOCK.code.size();
	emit(op);
	emit(static_cast<Op>(0xff));
	emit(static_cast<Op>(0xff));
	return index + 1;
}

void Compiler::patch_jump(std::size_t index) {
	u32 jump_dist = THIS_BLOCK.op_count() - index - 2;
	if (jump_dist > UINT16_MAX) {
		error_at("Too much code to jump over.", token.location.line);
	}

	// If we use a single opcode for the jump offset, then we're only allowed
	// to jump over 255 instructions, which is unfortunately a very small number.
	// To counter this, jumps are broken down into two Ops, the first Op contains the
	// first byte and the second Op contains the second byte. The bytes are then stitched
	// together to form a short int, thus allowing us to jump over UINT16_MAX bytes of code
	// (65535 instructions) at best.

	// For example, if we want to jump over 25000 opcodes, that means our jump offset
	// is `0x61A8`. The first byte, `0x61` goes in the first opcode, and the second
	// byte, `0xA8` goes in the second opcode. At runtime, the VM reads both of these,
	// and joins them together using some bit operators.
	THIS_BLOCK.code[index] = static_cast<Op>((jump_dist >> 8) & 0xff);
	THIS_BLOCK.code[index + 1] = static_cast<Op>(jump_dist & 0xff);
}

void Compiler::add_param(const Token& token) {
	new_variable(token);
	m_codeblock->add_param();
}

void Compiler::add_self_param() {
	SNAP_ASSERT(m_symtable.m_num_symbols == 1, "'self' must be the first parameter.");

	constexpr const char* self = "self";
	m_codeblock->add_param();
	m_symtable.add(self, 4, true);
}

// Helper functions:

void Compiler::advance() {
	prev = token;
	token = peek;
	peek = m_scanner->next_token();
}

bool Compiler::match(TT expected) noexcept {
	if (check(expected)) {
		advance();
		return true;
	}
	return false;
}

void Compiler::expect(TT expected, const char* err_msg) {
	if (check(expected)) {
		advance();
		return;
	}

	error_at_token(err_msg, token);
}

void Compiler::test(TT expected, const char* errmsg) {
	if (!check(expected)) {
		error_at_token(errmsg, peek);
	}
}

void Compiler::error_at(const char* message, u32 line) {
	ERROR("[line {}]: {}", line, message);
}

void Compiler::error_at_token(const char* message, const Token& token) {
	ERROR("[line {}]: near '{}': {}", token.location.line, token.raw(*m_source).c_str(), message);
}

void Compiler::error(std::string&& message) {
	m_vm->on_error(*m_vm, message);
	has_error = true;
	panic = true;
}

bool Compiler::ok() const {
	return !has_error;
}

u32 Compiler::emit_string(const Token& token) {
	u32 length = token.length() - 2; // minus the quotes
	// +1 to skip the openening quote.
	String& string = m_vm->string(token.raw_cstr(m_source) + 1, length);
	return emit_value(SNAP_OBJECT_VAL(&string));
}

u32 Compiler::emit_id_string(const Token& token) {
	String* s = &m_vm->string(token.raw(*m_source).c_str(), token.length());
	return emit_value(SNAP_OBJECT_VAL(s));
}

int Compiler::find_local_var(const Token& name_token) const noexcept {
	const char* name = name_token.raw_cstr(m_source);
	int length = name_token.length();
	const int idx = m_symtable.find(name, length);
	return idx;
}

int Compiler::find_upvalue(const Token& token) {
	if (m_parent == nullptr) return -1;

	// First search among the local variables of the enclosing
	// compiler.
	int index = m_parent->find_local_var(token);

	// If found the local var, then add it to the upvalues list.
	// and mark the upvalue is "local".
	if (index != -1) {
		LocalVar& local = m_parent->m_symtable.m_symbols[index];
		local.is_captured = true;
		return m_symtable.add_upvalue(index, true, local.is_const);
	}

	// If not found within the parent compiler's local vars
	// then look into the parent compiler's upvalues.
	index = m_parent->find_upvalue(token);

	// If found in some enclosing scope, add it to the current
	// upvalues list and return it.
	if (index != -1) {
		// is not local since we found it in an enclosing compiler.
		const UpvalDesc& upval = m_parent->m_symtable.m_upvals[index];
		return m_symtable.add_upvalue(index, false, upval.is_const);
	}

	// No local variable in any of the enclosing scopes was found with the same
	// name.
	return -1;
}

std::size_t Compiler::emit_value(Value v) {
	std::size_t index = THIS_BLOCK.add_value(v);
	if (index >= Compiler::MaxLocalVars) {
		error_at_token("Too many constants in one function.", token);
	}
	return index;
}

inline void Compiler::emit(Op op) {
	THIS_BLOCK.add_instruction(op, token.location.line);
}

inline void Compiler::emit(Op op, const Token& token) {
	THIS_BLOCK.add_instruction(op, token.location.line);
}

inline void Compiler::emit(Op op, u32 line) {
	THIS_BLOCK.add_instruction(op, line);
}

inline void Compiler::emit_bytes(Op a, Op b, const Token& token) {
	emit(a, token);
	emit(b, token);
}

inline void Compiler::emit_bytes(Op a, Op b, u32 line) {
	emit(a, line);
	emit(b, line);
}

inline void Compiler::emit(Op a, Op b) {
	emit(a, token);
	emit(b, token);
}

Op Compiler::toktype_to_op(TT toktype) const noexcept {
	switch (toktype) {
	case TT::Plus:
	case TT::PlusEq: return Op::add;
	case TT::Minus:
	case TT::MinusEq: return Op::sub;
	case TT::Div:
	case TT::DivEq: return Op::div;
	case TT::Mult:
	case TT::MultEq: return Op::mult;
	case TT::Mod:
	case TT::ModEq: return Op::mod;
	case TT::EqEq: return Op::eq;
	case TT::BangEq: return Op::neq;
	case TT::Concat: return Op::concat;
	case TT::BitLShift: return Op::lshift;
	case TT::BitRShift: return Op::rshift;
	case TT::BitAnd: return Op::band;
	case TT::BitOr: return Op::bor;
	case TT::Gt: return Op::gt;
	case TT::Lt: return Op::lt;
	case TT::GtEq: return Op::gte;
	case TT::LtEq: return Op::lte;
	default: return Op::op_count;
	}
}

bool Compiler::is_assign_tok(TT type) const noexcept {
	return (type == TT::Eq or (type >= TT::ModEq and type <= TT::PlusEq));
}

int Compiler::new_variable(const Token& varname, bool is_const) {
	const char* name = varname.raw_cstr(m_source);
	const u32 length = varname.length();

	// check of a variable with this name already exists in the current scope.
	if (m_symtable.find_in_current_scope(name, length) != -1) {
		ERROR("Attempt to redeclare existing variable '{}'.", varname.raw(*m_source));
		return -1;
	}

	return m_symtable.add(name, length, is_const);
}

// -- LocalVar Table --

int SymbolTable::add(const char* name, u32 length, bool is_const = false) {
	m_symbols[m_num_symbols] = LocalVar{name, length, u8(m_scope_depth), is_const};
	return m_num_symbols++;
}

static bool names_equal(const char* a, int len_a, const char* b, int len_b) {
	if (len_a != len_b) return false;
	return std::memcmp(a, b, len_a) == 0;
}

int SymbolTable::find(const char* name, int length) const {
	// start looking from the innermost scope, and work our way
	// outwards.
	for (int i = m_num_symbols - 1; i >= 0; i--) {
		const LocalVar& symbol = m_symbols[i];
		if (names_equal(name, length, symbol.name, symbol.length)) return i;
	}
	return -1;
}

int SymbolTable::find_in_current_scope(const char* name, int length) const {
	for (int i = m_num_symbols - 1; i >= 0; i--) {
		const LocalVar& symbol = m_symbols[i];
		if (symbol.depth < m_scope_depth) return -1; // we've reached an outer scope
		if (names_equal(name, length, symbol.name, symbol.length)) return i;
	}
	return -1;
}

const LocalVar* SymbolTable::find_by_slot(const u8 index) const {
	return &m_symbols[index];
}

int SymbolTable::add_upvalue(int index, bool is_local, bool is_const) {
	// If the upvalue has already been captured, then return the stored value.
	for (int i = 0; i < m_num_upvals; ++i) {
		UpvalDesc& upval = m_upvals[i];
		if (upval.index == index and upval.is_local == is_local) {
			return i;
		}
	}

	m_upvals[m_num_upvals] = UpvalDesc{index, is_const, is_local};
	return m_num_upvals++;
}

} // namespace snap