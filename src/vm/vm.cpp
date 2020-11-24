#include "vm.hpp"
#include "../common.hpp"
#include "../compiler/compiler.hpp"
#include "../syntax/parser.hpp"

#if defined(SNAP_DEBUG_RUNTIME) || defined(SNAP_DEBUG_DISASSEMBLY)
#include "../debug.hpp"
#endif

#define NEXT_OP()				(m_block.code[ip++])
#define NEXT_BYTE()				((u8)(m_block.code[ip++]))
#define READ_VALUE()			(m_block.constant_pool[NEXT_BYTE()])
#define SET_VALUE(depth, value) (m_stack[sp - depth - 1] = value)
#define GET_VAR(index)			(m_stack[index])
#define SET_VAR(index, value)	(m_stack[index] = value)

namespace snap {
using Op = Opcode;

VM::VM(const std::string* src) : source{src}, m_block{Block{}} {};

#define BINOP(op)                                                                                  \
	do {                                                                                           \
		m_stack[sp - 2] = m_stack[sp - 2] op m_stack[sp - 1];                                      \
		pop();                                                                                     \
	} while (false)

#ifdef SNAP_DEBUG_RUNTIME
void print_stack(Value stack[VM::StackMaxSize], size_t sp) {
	printf("(%zu) [", sp);
	for (Value* v = stack; v < stack + sp; v++) {
		print_value(*v);
		printf(" ");
	}
	printf("]\n");
}
#endif

ExitCode VM::run(bool run_till_end) {

	do {
		const Op op = NEXT_OP();
		switch (op) {
		case Op::load_const: push(READ_VALUE()); break;
		case Op::pop: pop(); break;
		case Op::add: BINOP(+); break;
		case Op::sub: BINOP(-); break;
		case Op::mult: BINOP(*); break;
		case Op::div: BINOP(/); break;
		case Op::get_var: {
			u8 idx = NEXT_BYTE();
			push(GET_VAR(idx));
			break;
		}
		case Op::set_var: {
			u8 idx = NEXT_BYTE();
			SET_VALUE(idx, peek(0));
			break;
		}
		case Op::return_val: return ExitCode::Success;
		default: std::cout << "not implemented yet" << std::endl;
		}
#ifdef SNAP_DEBUG_RUNTIME
		printf("%-4s   ", op2s(op));
		print_stack(m_stack, sp);
#endif
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

#ifdef SNAP_DEBUG_DISASSEMBLY
	disassemble_block(m_block);
	printf("\n");
#endif

	return true;
}

} // namespace snap