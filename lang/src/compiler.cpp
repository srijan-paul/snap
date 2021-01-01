#include "token.hpp"
#include <compiler.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>


#define TOK2NUM(t)		   SNAP_NUM_VAL(std::stof(t.raw(*m_source)))
#define SPTR_CAST(type, v) static_cast<const type*>(v)

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
	emit(Op::return_val, -1);
}

// top level statements are one of:
// - var declaration
// - function declaration
// - assignment
// - call expr
// - export statement
void Compiler::toplevel() {
	if (has_error) recover();
	if (eof()) return;

	if (match(TT::Let)) {
		return var_decl();
	} else {
		expr_stmt();
	}
}

/// @brief compiles a variable declaration assuming a `let` or `const`
/// has been consumed.
void Compiler::var_decl() {
	do {
		declarator();
	} while (match(TT::Comma));
	match(TT::Semi); // optional semi colon at the end.
}

void Compiler::declarator() {
	expect(TT::Id, "Expected variable name.");
	// add the new variable to the symbol table.
	new_variable(token);
	// default value is `nil`.
	match(TT::Eq) ? expr() : emit(Op::load_nil, token.location.line);
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
	logic_and(true);
}

void Compiler::logic_and(bool can_assign) {
	bit_or(true);
}

DEFINE_PARSE_FN(Compiler::bit_or, match(TT::BitOr), bit_and)
DEFINE_PARSE_FN(Compiler::bit_and, match(TT::BitAnd), equality)
DEFINE_PARSE_FN(Compiler::equality, match(TT::EqEq) || match(TT::BangEq), comparison)
DEFINE_PARSE_FN(Compiler::comparison, match(TT::BitLShift) || match(TT::BitRShift), b_shift)
DEFINE_PARSE_FN(Compiler::b_shift,
				match(TT::Gt) || match(TT::Lt) || match(TT::GtEq) || match(TT::LtEq), sum)
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
	} else if (match(TT::Id)) {
		const size_t index = find_var(token);
		if (can_assign && match(TT::Eq)) {
			expr(); // compile assignment RHS
			emit_bytes(Op::set_var, static_cast<Op>(index), token);
		} else {
			emit_bytes(Op::get_var, static_cast<Op>(index), token);
		}
	} else {
		const std::string raw = peek.raw(*m_source);
		const char fmt[] = "Unexpected '%s'.";
		std::size_t bufsize = strlen(fmt) + raw.length() - 1;
		char buf[bufsize];
		sprintf(buf, fmt, raw.c_str());
		error_at(buf, peek.location.line);
	}
}

void Compiler::literal() {
	advance();
	std::size_t index;
	switch (token.type) {
	case TT::Integer:
	case TT::Float: index = emit_value(TOK2NUM(token)); break;
	case TT::String: index = emit_string(token); break;
	default:;
	}

	if (index >= UINT8_MAX) {
		error_at_token("Too many literal constants in one function.", token);
	}

	emit_bytes(Op::load_const, static_cast<Op>(index), token);
}

void Compiler::recover() {
	while (!(eof() || match(TT::Semi) || match(TT::LCurlBrace) || match(TT::LSqBrace))) {
		advance();
	}
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
	return m_vm->m_block.add_value(v);
}

inline void Compiler::emit(Op op, const Token& token) {
	m_vm->m_block.add_instruction(op, token.location.line);
}

inline void Compiler::emit(Op op, u32 line) {
	m_vm->m_block.add_instruction(op, line);
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
	default: return Op::op_count;
	}
}

int Compiler::new_variable(const Token& varname) {
	const char* name = varname.raw_cstr(m_source);
	const u32 length = varname.length();

	if (m_symtable.find_in_current_scope(name, length) != -1) {
		error_at("Attempt to redeclare variable", varname.location.line);
		return -1;
	}

	return m_symtable.add(name, length);
}

// -- Symbol Table --

int SymbolTable::add(const char* name, u32 length) {
	symbols[num_symbols++] = Symbol(name, length, scope_depth);
	return num_symbols - 1;
}

static bool names_equal(const char* a, int len_a, const char* b, int len_b) {
	if (len_a != len_b) return false;
	return std::memcmp(a, b, len_a) == 0;
}

int SymbolTable::find(const char* name, int length) const {
	for (int i = num_symbols - 1; i >= 0; i--) {
		const Symbol* symbol = &symbols[i];
		if (names_equal(name, length, symbol->name, symbol->length)) return i;
	}
	return -1;
}

int SymbolTable::find_in_current_scope(const char* name, int length) const {
	for (int i = num_symbols - 1; i >= 0; i--) {
		const Symbol* symbol = &symbols[i];
		if (symbol->depth < scope_depth) return -1;
		if (names_equal(name, length, symbol->name, symbol->length)) return i;
	}
	return -1;
}

} // namespace snap