#include "vm.hpp"
#include "../common.hpp"
#include "../compiler/compiler.hpp"
#include "../syntax/parser.hpp"

#define NEXT_OP()				(m_block.code[ip++])
#define NEXT_BYTE()				((u8)(m_block.code[ip++]))
#define GET_VALUE()				(m_block.constant_pool[NEXT_BYTE()])
#define SET_VALUE(depth, value) (m_stack[sp - depth - 1] = value)

namespace snap {
using Op = Opcode;

VM::VM(const std::string* src) : source{src}, m_block{Block{}} {};

#define BINOP(op)                                                                                  \
	do {                                                                                           \
		m_stack[sp - 2] = m_stack[sp - 2] op m_stack[sp - 1];                                      \
		pop();                                                                                     \
	} while (false)

ExitCode VM::run(bool run_till_end) {
	do {
		const Op op = NEXT_OP();
		switch (op) {
		case Op::push: push(GET_VALUE()); break;
		case Op::pop: pop(); break;
		case Op::add: BINOP(+); break;
		case Op::sub: BINOP(-); break;
		case Op::mult: BINOP(*); break;
		case Op::div: BINOP(/); break;
		default: std::cout << "not implemented yet" << std::endl;
		}
	} while (run_till_end);

	return ExitCode::Success;
}

ExitCode VM::interpret() {
	init();
	return run();
}

bool VM::init() {
	Parser parser{source};
	Program* ast = parser.parse();

	Compiler compiler{&m_block, ast, source};
	compiler.compile();
	delete ast; // TODO use std::unique_ptr
	return true;
}

} // namespace snap