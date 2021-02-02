#include "debug.hpp"
#include "token.hpp"
#include "value.hpp"
#include <compiler.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <vm.hpp>

#define TOK2NUM(t) SNAP_NUM_VAL(std::stod(t.raw(*m_source)))
#define THIS_BLOCK (m_proto->m_block)

#define DEFINE_PARSE_FN(name, cond, next_fn)                                                       \
	void name(bool can_assign) {                                                                   \
		next_fn(can_assign);                                                                       \
		while (cond) {                                                                             \
			Token op_token = token;                                                                \
			next_fn(false);                                                                        \
			emit(toktype_to_op(op_token.type), op_token);                                          \
		}                                                                                          \
	}

namespace snap {

using Op = Opcode;
using TT = TokenType;

Compiler::Compiler(VM* vm, const std::string* src) : m_vm{vm}, m_source{src} {
	m_scanner = new Scanner{src};
	advance(); // set `peek` to the first token in the token stream.
	String* fname = &vm->make<String>("<script>", 8);
	// reserve the first slot for this toplevel function.
	m_symtable.add("<script>", 8, true);
	m_proto = &vm->make<Prototype>(fname);
}

Compiler::Compiler(VM* vm, Compiler* parent, const String* name) : m_vm{vm}, m_parent{parent} {
	m_scanner = m_parent->m_scanner;
	m_proto = &m_vm->make<Prototype>(name);

	m_symtable.add(name->c_str(), name->m_length, false);

	m_source = parent->m_source;
	vm->m_compiler = this;

	prev = parent->prev;
	token = parent->token;
	peek = parent->peek;
}

Compiler::~Compiler() {
	if (m_parent == nullptr) {
		delete m_scanner;
	}
}

Prototype* Compiler::compile() {
	while (!eof()) {
		toplevel();
	}

	m_proto->m_num_upvals = m_symtable.num_upvals;
	emit_bytes(Op::load_nil, Op::return_val, token.location.line);
	return m_proto;
}

Prototype* Compiler::compile_func() {
	test(TT::LCurlBrace, "Expected '{' before function body.");

	block_stmt();
	m_proto->m_num_upvals = m_symtable.num_upvals;
	emit(Op::load_nil);
	emit(Op::return_val);
	m_vm->m_compiler = m_parent;
	return m_proto;
}

// top level statements are one of:
// - var declaration
// - function declaration
// - expression statement
// - export statement
void Compiler::toplevel() {
	if (has_error) recover();
	if (eof()) return;

	const auto tt = peek.type;

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
	while (!(eof() || check(TT::RCurlBrace))) {
		toplevel();
	}
	expect(TT::RCurlBrace, "Expected '}' to close block.");
	exit_block();
}

void Compiler::if_stmt() {
	advance(); // consume 'if'
	expr();	   // parse condition.
	std::size_t jmp = emit_jump(Op::pop_jmp_if_false);
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

	Token name_token = token;
	String* fname = &m_vm->make<String>(name_token.raw_cstr(m_source), name_token.length());

	func_expr(fname);
	new_variable(name_token);
}

/// Compiles the body of a function assuming everything until the
/// the opening parenthesis has been consumed.
void Compiler::func_expr(const String* fname) {
	Compiler compiler{m_vm, this, fname};

	compiler.expect(TT::LParen, "Expected '(' before function parameters.");

	if (!compiler.check(TT::RParen)) {
		do {
			compiler.expect(TT::Id, "Expected parameter name.");
			compiler.add_param(compiler.token);
		} while (compiler.match(TT::Comma));
	}

	compiler.expect(TT::RParen, "Expected ')' after function parameters.");

	Prototype* proto = compiler.compile_func();
	u8 idx = emit_value(SNAP_OBJECT_VAL(proto));

	emit_bytes(Op::make_func, static_cast<Op>(idx), token);
	emit(static_cast<Op>(proto->m_num_upvals), token);

	for (int i = 0; i < compiler.m_symtable.num_upvals; ++i) {
		CompilerUpval& upval = compiler.m_symtable.m_upvals[i];
		emit(upval.is_local ? static_cast<Op>(1) : static_cast<Op>(0));
		emit(static_cast<Op>(upval.index));
	}

#ifdef SNAP_DEBUG_DISASSEMBLY
	disassemble_block(proto->name_cstr(), proto->m_block);
#endif

	prev = compiler.prev;
	token = compiler.token;
	peek = compiler.peek;
}

void Compiler::ret_stmt() {
	advance(); // eat the return keyword.
	if (isLiteral(peek.type) || check(TT::Id) || check(TT::Bang) || check(TT::Minus) ||
		check(TT::LParen) || check(TT::Fn)) {
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

void Compiler::expr() {
	logic_or(true);
}

void Compiler::logic_or(bool can_assign) {
	logic_and(can_assign);
	if (match(TT::Or)) {
		std::size_t jump = emit_jump(Op::jmp_if_true_or_pop);
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
DEFINE_PARSE_FN(Compiler::equality, match(TT::EqEq) || match(TT::BangEq), comparison)
DEFINE_PARSE_FN(Compiler::comparison,
				match(TT::Gt) || match(TT::Lt) || match(TT::GtEq) || match(TT::LtEq), b_shift)
DEFINE_PARSE_FN(Compiler::b_shift, match(TT::BitLShift) || match(TT::BitRShift), sum)
DEFINE_PARSE_FN(Compiler::sum, (match(TT::Plus) || match(TT::Minus) || match(TT::Concat)), mult)
DEFINE_PARSE_FN(Compiler::mult, (match(TT::Mult) || match(TT::Mod) || match(TT::Div)), unary)

void Compiler::unary(bool can_assign) {
	if (check(TT::Bang) || check(TT::Minus)) {
		advance();
		const Token op_token = token;
		call(false);
		switch (op_token.type) {
		case TT::Bang: emit(Op::lnot, op_token); break;
		case TT::Minus: emit(Op::negate, op_token); break;
		default:;
		}
		return;
	}
	call(can_assign);
}

void Compiler::call(bool can_assign) {
	grouping(can_assign);

	while (!eof() and match(TT::LParen)) {
		u8 argc = 0;

		if (!check(TT::RParen)) {
			do {
				argc++;
				expr(); // push the arguments on the stack,
			} while (match(TT::Comma));
		}

		expect(TT::RParen, "Expected ')' after call.");
		emit_bytes(Op::call_func, static_cast<Op>(argc), token.location.line);
	}
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
		constexpr const char* name = "<anonymous>";
		const String* fname = &m_vm->make<String>(name, 11);
		if (check(TT::LParen)) return func_expr(fname);
		// Names of lambda expressions are simply ignored
		// Unless found in a statement context.
		expect(TT::Id, "Expected function name or '('.");
		return func_expr(fname);
	} else if (check(TT::Id)) {
		variable(can_assign);
	} else {
		const std::string raw = peek.raw(*m_source);
		const char fmt[] = "Unexpected '%s'.";
		std::size_t bufsize = strlen(fmt) + raw.length() - 1;
		std::unique_ptr<char[]> buf{new char[bufsize]};
		sprintf(buf.get(), fmt, raw.c_str());
		error_at(buf.get(), peek.location.line);
	}
}

void Compiler::variable(bool can_assign) {
	advance();

	Op get_op = Op::get_var, set_op = Op::set_var;

	int index = find_local_var(token);
	bool is_const = (index == -1) ? false : m_symtable.find_by_slot(index)->is_const;

	// if no local variable with that name was found then look for an
	// upvalue.
	if (index == -1) {
		index = find_upvalue(token);
		get_op = Opcode::get_upval;
		set_op = Opcode::set_upval;
		is_const = (index == -1) ? false : m_symtable.m_upvals[index].is_const;
	}

	if (can_assign && match(TT::Eq)) {
		if (is_const) {
			std::string message{"Cannot assign to variable '"};
			message = message + prev.raw(*m_source) + "' which is marked 'const'.";
			error_at_token(message.c_str(), token);
			has_error = false; // don't send the compiler into error recovery mode.
		}

		expr(); // compile assignment RHS
		emit_bytes(set_op, static_cast<Op>(index), token);
	} else {
		emit_bytes(get_op, static_cast<Op>(index), token);
	}
}

void Compiler::literal() {
	advance();
	std::size_t index;
	switch (token.type) {
	case TT::Integer:
	case TT::Float: index = emit_value(TOK2NUM(token)); break;
	case TT::String: index = emit_string(token); break;
	case TT::True: index = emit_value(SNAP_BOOL_VAL(true)); break;
	case TT::False: index = emit_value(SNAP_BOOL_VAL(false)); break;
	case TT::Nil: index = emit_value(SNAP_NIL_VAL); break;
	default:;
	}

	if (index >= Compiler::MaxLocalVars) {
		error_at_token("Too many literal constants in one function.", token);
	}

	emit_bytes(Op::load_const, static_cast<Op>(index), token);
}

void Compiler::recover() {
	while (!(eof() || match(TT::Semi) || match(TT::LCurlBrace) || match(TT::LSqBrace))) {
		advance();
	}
}

void Compiler::enter_block() {
	m_symtable.m_scope_depth++;
}

void Compiler::exit_block() {
	for (int i = m_symtable.num_symbols - 1; i >= 0; i--) {
		const Symbol& var = m_symtable.m_symbols[i];

		if (var.depth != m_symtable.m_scope_depth) break;

		emit(var.is_captured ? Op::close_upval : Op::pop);
		m_symtable.num_symbols--;
	}

	m_symtable.m_scope_depth--;
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

	THIS_BLOCK.code[index] = static_cast<Op>((jump_dist >> 8) & 0xff);
	THIS_BLOCK.code[index + 1] = static_cast<Op>(jump_dist & 0xff);
}

void Compiler::add_param(const Token& token) {
	new_variable(token);
	m_proto->m_num_params++;
	if (m_proto->m_num_params > MaxFuncParams) {
		error_at_token("Too many function parameters.", token);
	}
}

// helper functions:

void Compiler::advance() {
	prev = token;
	token = peek;
	peek = m_scanner->next_token();
}

bool Compiler::match(TT expected) {
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

void Compiler::error_at(const char* fmt, u32 line) {
	error("[line %d]: %s", line, fmt);
}

void Compiler::error_at_token(const char* message, const Token& token) {
	error("[line %d]: near '%s': %s", token.location.line, token.raw(*m_source).c_str(), message);
}

void Compiler::error(const char* fmt...) {
	has_error = true;

	va_list args;
	va_start(args, fmt);

	std::size_t bufsize = vsnprintf(nullptr, 0, fmt, args) + 1;

	std::unique_ptr<char[]> buf{new char[bufsize]};

	vsnprintf(buf.get(), bufsize, fmt, args);
	m_vm->log_error(*m_vm, buf.get());

	va_end(args);
}

size_t Compiler::emit_string(const Token& token) {
	size_t length = token.length() - 2; // minus the quotes
	// +1 to skip the openening quote.
	String& string = m_vm->make<String>(token.raw_cstr(m_source) + 1, length);
	return emit_value(SNAP_OBJECT_VAL(&string));
}

int Compiler::find_local_var(const Token& name_token) {
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
		Symbol& symbol = m_parent->m_symtable.m_symbols[index];
		symbol.is_captured = true;
		return m_symtable.add_upvalue(index, true, symbol.is_const);
	}

	// If not found within the parent compiler's local vars
	// then look into the parent compiler's upvalues.
	index = m_parent->find_upvalue(token);

	// If found in some enclosing scope, add it to the current
	// upvalues list and return it.
	if (index != -1) {
		// is not local since we found it in an enclosing compiler.
		return m_symtable.add_upvalue(index, false, m_parent->m_symtable.m_upvals[index].is_const);
	}

	return -1;
}

inline size_t Compiler::emit_value(Value v) {
	return THIS_BLOCK.add_value(v);
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

Op Compiler::toktype_to_op(TT toktype) {
	switch (toktype) {
	case TT::Plus: return Op::add;
	case TT::Minus: return Op::sub;
	case TT::Div: return Op::div;
	case TT::Mult: return Op::mult;
	case TT::Mod: return Op::mod;
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

int Compiler::new_variable(const Token& varname, bool is_const) {
	const char* name = varname.raw_cstr(m_source);
	const u32 length = varname.length();

	// check of a variable with this name already exists in the current scope.
	if (m_symtable.find_in_current_scope(name, length) != -1) {
		error_at("Attempt to redeclare existing variable.", varname.location.line);
		return -1;
	}

	return m_symtable.add(name, length, is_const);
}

// -- Symbol Table --

int SymbolTable::add(const char* name, u32 length, bool is_const = false) {
	m_symbols[num_symbols++] = Symbol(name, length, m_scope_depth, is_const);
	return num_symbols - 1;
}

static bool names_equal(const char* a, int len_a, const char* b, int len_b) {
	if (len_a != len_b) return false;
	return std::memcmp(a, b, len_a) == 0;
}

int SymbolTable::find(const char* name, int length) const {
	// start looking from the innermost scope, and work our way
	// upwards.
	for (int i = num_symbols - 1; i >= 0; i--) {
		const Symbol& symbol = m_symbols[i];
		if (names_equal(name, length, symbol.name, symbol.length)) return i;
	}
	return -1;
}

int SymbolTable::find_in_current_scope(const char* name, int length) const {
	for (int i = num_symbols - 1; i >= 0; i--) {
		const Symbol& symbol = m_symbols[i];
		if (symbol.depth < m_scope_depth) return -1; // we've reached an outer scope
		if (names_equal(name, length, symbol.name, symbol.length)) return i;
	}
	return -1;
}

Symbol* SymbolTable::find_by_slot(const u8 index) {
	return &m_symbols[index];
}

int SymbolTable::add_upvalue(u8 index, bool is_local, bool is_const) {
	// If the upvalue has already been cached, then return the stored value.
	for (int i = 0; i < num_upvals; ++i) {
		CompilerUpval& upval = m_upvals[i];
		if (upval.index == index and upval.is_local == is_local) {
			return i;
		}
	}

	CompilerUpval& upval = m_upvals[num_upvals];

	upval.index = index;
	upval.is_local = is_local;
	upval.is_const = is_const;
	return num_upvals++;
}

} // namespace snap