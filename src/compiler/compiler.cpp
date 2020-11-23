#include "compiler.hpp"
#include <cstring>
#include <string>

#define TOK2INT(t) std::stoi(t->raw(*source))
#define TOK2FLT(t) std::stof(t->raw(*source))

namespace snap {
using Op = Opcode;
using TT = TokenType;

void Compiler::compile() {
	for (auto s : m_ast->stmts) {
		compile_stmt(s);
	}
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

void Compiler::compile_vardecl(const VarDecl* decl) {
	for (auto var : decl->declarators) {
		if (var->init != nullptr) {
			compile_exp(var->init);
		} else {
			emit(Op::nil);
		}
	}
}

void Compiler::compile_exp(const Expr* exp) {
	switch (exp->type) {
	case NodeType::BinExpr: compile_binexp((BinExpr*)exp); break;
	case NodeType::Literal: compile_literal((Literal*)exp); break;
	default:;
	}
}

void Compiler::compile_binexp(const BinExpr* exp) {
	switch (exp->token.type) {
	case TT::Plus:
	case TT::Minus:
	case TT::Mult:
	case TT::Mod:
		compile_exp(exp->left);
		compile_exp(exp->right);
		emit(toktype_to_op(exp->token.type));
		return;
	default:;
	}
}

void Compiler::compile_literal(const Literal* literal) {
	const Token* t = &literal->token;
	size_t index;
	switch (literal->token.type) {
	case TT::Integer: index = emit_value(TOK2INT(t)); break;
	case TT::Float: index = emit_value(TOK2FLT(t)); break;
	default:;
	}
	if (index > UINT8_MAX) {
		// TODO: ERROR
	}

	emit(Op::load_const, (Op)index);
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
	default: return Op::op_count;
	}
}

/* --- Symbol Table --- */

int SymbolTable::add(const char* name, u32 length) {
	if (find_in_current_scope(name, length) != -1) return -1;
	symbols[num_symbols] = Symbol(name, length, scope_depth);
	return num_symbols++;
}

static bool names_equal(const char* a, int len_a, const char* b, int len_b) {
	if (len_a != len_b) return false;
	return std::memcmp(a, b, len_a) == 0;
}

int SymbolTable::find(const char* name, int length) const {
	for (size_t i = num_symbols; i >= 0; i--) {
		const Symbol* symbol = &symbols[i];
		if (names_equal(name, length, symbol->name, symbol->length)) return i;
	}
	return -1;
}

int SymbolTable::find_in_current_scope(const char* name, int length) const {
	for (size_t i = num_symbols; i >= 0; i--) {
		const Symbol* symbol = &symbols[i];
		if (symbol->depth < scope_depth) return -1;
		if (names_equal(name, length, symbol->name, symbol->length)) return i;
	}
	return -1;
}

} // namespace snap