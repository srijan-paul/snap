#include "token.hpp"
#include "value.hpp"
#include <compiler.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>

#define TOK2NUM(t) SNAP_NUM_VAL(std::stod(t.raw(*m_source)))
#define THIS_BLOCK (m_vm->m_block)

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

Compiler::Compiler(VM* vm, const std::string* src)
	: m_vm{vm}, m_source{src}, m_scanner(Scanner{src}) {
	advance(); // set `peek` to the first token in the token stream.
}

void Compiler::compile() {
	while (!eof()) {
		toplevel();
	}
	emit(Op::return_val, token);
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
	int idx = new_variable(name, is_const);
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
	grouping(can_assign);
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
	} else if (check(TT::Id)) {
		variable(can_assign);
	} else {
		const std::string raw = peek.raw(*m_source);
		const char fmt[] = "Unexpected '%s'.";
		std::size_t bufsize = strlen(fmt) + raw.length() - 1;
		char buf[bufsize];
		sprintf(buf, fmt, raw.c_str());
		error_at(buf, peek.location.line);
	}
}

void Compiler::variable(bool can_assign) {
	advance();
	const size_t index = find_var(token);

	if (can_assign && match(TT::Eq)) {
		const Symbol* local = m_symtable.find_by_slot(index);
		if (local->is_const) {
			std::string message{"Cannot assign to variable '"};
			message = message + prev.raw(*m_source) + "' which is marked 'const'.";
			error_at_token(message.c_str(), token);
			has_error = false; // don't sen the compiler into error recovery mode.
		}

		expr(); // compile assignment RHS
		emit_bytes(Op::set_var, static_cast<Op>(index), token);
	} else {
		emit_bytes(Op::get_var, static_cast<Op>(index), token);
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
	m_symtable.scope_depth++;
}

void Compiler::exit_block() {
	for (int i = m_symtable.num_symbols - 1; i >= 0; i--) {
		const Symbol& var = m_symtable.m_symbols[i];

		if (var.depth != m_symtable.scope_depth) break;

		emit(Opcode::pop, token);
		m_symtable.num_symbols--;
	}

	m_symtable.scope_depth--;
}

std::size_t Compiler::emit_jump(Opcode op) {
	std::size_t index = THIS_BLOCK.code.size();
	emit(op, token);
	emit(static_cast<Op>(0xff), token);
	emit(static_cast<Op>(0xff), token);
	return index + 1;
}

void Compiler::patch_jump(std::size_t index) {
	u16 jump_dist = THIS_BLOCK.op_count() - index - 2;
	if (jump_dist > UINT16_MAX) {
		error_at("Too much code to jump over.", token.location.line);
	}

	THIS_BLOCK.code[index] = static_cast<Op>((jump_dist >> 8) & 0xff);
	THIS_BLOCK.code[index + 1] = static_cast<Op>(jump_dist & 0xff);
}

// helper functions:

void Compiler::advance() {
	prev = token;
	token = peek;
	peek = m_scanner.next_token();
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
	char buf[bufsize];
	vsnprintf(buf, bufsize, fmt, args);

	m_vm->log_error(*m_vm, buf);

	va_end(args);
}

size_t Compiler::emit_string(const Token& token) {
	size_t length = token.length() - 2; // minus the quotes
	char* buf = new char[length + 1];
	std::memcpy(buf, token.raw_cstr(m_source) + 1, length); // +1 to skip the openening quote.
	buf[length] = '\0';
	return emit_value(Value(buf, length));
}

int Compiler::find_var(const Token& name_token) {
	const char* name = name_token.raw_cstr(m_source);
	int length = name_token.length();
	const int idx = m_symtable.find(name, length);
	return idx;
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

	// check of a varaible with this name already exists in the current scope.
	if (m_symtable.find_in_current_scope(name, length) != -1) {
		error_at("Attempt to redeclare existing variable.", varname.location.line);
		return -1;
	}

	return m_symtable.add(name, length, is_const);
}

// -- Symbol Table --

int SymbolTable::add(const char* name, u32 length, bool is_const = false) {
	m_symbols[num_symbols++] = Symbol(name, length, scope_depth, is_const);
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
		if (symbol.depth < scope_depth) return -1; // we've reached an outer scope
		if (names_equal(name, length, symbol.name, symbol.length)) return i;
	}
	return -1;
}

Symbol* SymbolTable::find_by_slot(const u8 index) {
	return &m_symbols[index];
}

} // namespace snap