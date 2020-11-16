#include "compiler.hpp"
#include <string>

#define TOK2INT(t) std::stoi(t->raw(*source))
#define TOK2FLT(t) std::stof(t->raw(*source))

namespace snap {
using Op = Opcode;
using TT = TokenType;

void Compiler::compile() {
}

void Compiler::compile_exp(const Expr* exp) {
	switch (exp->type) {
	case NodeType::BinExpr: compile_binexp((BinExpr*)exp);
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
	switch (literal->token.type) {
	case TT::Integer: emit_value(TOK2INT(t));
	case TT::Float: emit_value(TOK2FLT(t));
	default:;
	}
}

inline void Compiler::emit_value(Value v) {
	m_block->add_value(v);
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
	default: return Op::error;
	}
}

} // namespace snap