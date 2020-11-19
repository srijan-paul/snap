#include "vm.hpp"
#include "../common.hpp"

#define NEXT_OP()				(m_block->code[ip++])
#define NEXT_BYTE()				((u8)(m_block->code[ip++]))
#define GET_VALUE()				(m_block->constant_pool[NEXT_BYTE()])
#define SET_VALUE(depth, value) (m_stack[sp - depth - 1] = value)

namespace snap {
using Op = Opcode;

#define BINOP(op)                                                                                  \
	do {                                                                                           \
		m_stack[sp - 2] = m_stack[sp - 2] op m_stack[sp - 1];                                      \
		pop();                                                                                     \
	} while (false)

ExitCode VM::run(bool run_once) {
	do {
		const Op op = NEXT_OP();
		switch (op) {
		case Op::push: push(GET_VALUE());
		case Op::pop: pop();
		case Op::add: BINOP(+);
		case Op::sub: BINOP(-);
		case Op::mult: BINOP(*);
		case Op::div: BINOP(/);
		default: std::cout << "not implemented yet" << std::endl;
		}
	} while (run_once);

	return ExitCode::Success;
}

} // namespace snap