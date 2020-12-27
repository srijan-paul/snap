#include "compiler.hpp"
#include <cstring>
#include <string>

#define TOK2INT(t) SNAP_INT_VAL(std::stoi(t.raw(*source)))
#define TOK2FLT(t) SNAP_FLOAT_VAL(std::stof(t.raw(*source)))

namespace snap {
using Op = Opcode;
using TT = TokenType;

void Compiler::compile() {
	for (auto s : m_ast->stmts) {
		compile_stmt(s);
	}
	emit(Op::return_val);
}

void Compiler::compile_stmt(const Stmt* s) {
	switch (s->type) {
	case NodeType::ExprStmt:
		compile_exp(((ExprStmt*)s)->exp);
		emit(Op::pop);
		break;
	case NodeType::VarDeclaration: compile_vardecl((VarDecl*)s); break;
	default:;
	}
}

void Compiler::compile_vardecl(const VarDecl* stmt) {
	for (auto decl : stmt->declarators) {
		new_variable(&decl->var);

		if (decl->init != nullptr) {
			compile_exp(decl->init);
		} else {
			emit(Op::nil);
		}
	}
}

void Compiler::compile_exp(const Expr* exp) {
	switch (exp->type) {
	case NodeType::BinExpr: compile_binexp((BinExpr*)exp); break;
	case NodeType::Literal: compile_literal((Literal*)exp); break;
	case NodeType::VarId: compile_var((VarId*)exp); break;
	default:;
	}
}

void Compiler::compile_binexp(const BinExpr* exp) {
	switch (exp->token.type) {
	case TT::Eq:
		switch (exp->left->type) {
		case NodeType::VarId: {
			compile_exp(exp->right);
			emit(Op::set_var, (Op)find_var(&exp->left->token));
			break;
		}
		default: return;
		}
		break;
	case TT::Plus:
	case TT::Minus:
	case TT::Mult:
	case TT::Mod:
	case TT::Concat:
		compile_exp(exp->left);
		compile_exp(exp->right);
		emit(toktype_to_op(exp->token.type));
		return;
	default:;
	}
}

size_t Compiler::emit_string(const Token& token) {
	size_t length = token.length() - 2; // minus the quotes
	char* buf = new char[length + 1];
	std::memcpy(buf, token.raw_cstr(source) + 1, length); // +1 to skip the openening quote.
	buf[length] = '\0';
	return emit_value(Value(buf, length));
}

void Compiler::compile_literal(const Literal* literal) {
	const Token& t = literal->token;
	size_t index;
	switch (literal->token.type) {
	case TT::Integer: index = emit_value(TOK2INT(t)); break;
	case TT::Float: index = emit_value(TOK2FLT(t)); break;
	case TT::String: index = emit_string(t); break;
	default:;
	}

	if (index >= UINT8_MAX) {
		// TODO: Too many constants error.
	}

	emit(Op::load_const, (Op)index);
}

int Compiler::find_var(const Token* name_token) {
	const char* name = name_token->raw_cstr(source);
	int length = name_token->length();
	const int idx = symbol_table.find(name, length);
	if (idx == -1) { /* TODO: Reference error */
	}
	return idx;
}

void Compiler::compile_var(const VarId* var) {
	emit(Op::get_var, (Op)find_var(&var->token));
}

inline size_t Compiler::emit_value(Value v) {
	return m_block->add_value(v);
}

inline void Compiler::emit(Op op) {
	m_block->add_instruction(op);
}

inline void Compiler::emit(Op a, Op b) {
	emit(a);
	emit(b);
}

Op Compiler::toktype_to_op(TT toktype) {
	switch (toktype) {
	case TT::Plus: return Op::add;
	case TT::Minus: return Op::sub;
	case TT::Mult: return Op::mult;
	case TT::Mod: return Op::mod;
	case TT::EqEq: return Op::eq;
	case TT::Concat: return Op::concat;
	default: return Op::op_count;
	}
}

int Compiler::new_variable(const Token* varname) {
	const char* name = varname->raw_cstr(source);
	const u32 length = varname->length();

	if (symbol_table.find_in_current_scope(name, length) != -1) {
		// TODO throw error
		return -1;
	}

	return symbol_table.add(name, length);
}

/* --- Symbol Table --- */

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