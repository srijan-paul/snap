#include "vm.hpp"
#include "../common.hpp"
#include "../compiler/compiler.hpp"
#include "../syntax/parser.hpp"
#include <cmath>

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
using VT = ValueType;

static bool try_float_cast(Value& v, double* d) {
	if (v.is_float()) {
		*d = v.as_float();
		return true;
	}

	if (v.is_int()) {
		*d = v.as_int();
		v.tag = VT::Float;
		return true;
	}

	return false;
}

static bool try_int_cast(Value& v, int* iptr) {
	if (v.is_int()) {
		*iptr = v.as_int();
		return true;
	}

	if (v.is_float()) {
		*iptr = v.as_float();
		v.tag = VT::Int;
		return true;
	}

	return false;
}

VM::VM(const std::string* src) : source{src}, m_block{Block{}} {};

// TODO: errors
#define BINOP(op)                                                                                  \
	do {                                                                                           \
		Value& a = m_stack[sp - 1];                                                                \
		Value& b = m_stack[sp - 2];                                                                \
                                                                                                   \
		if (a.is_float()) {                                                                        \
			if (b.is_float()) {                                                                    \
				b.as.float_ = a.as_float() op b.as_float();                                        \
			} else if (b.is_int()) {                                                               \
				b.set_float(a.as_float() op b.as_int());                                           \
			}                                                                                      \
		} else if (a.is_int()) {                                                                   \
			if (b.is_int()) {                                                                      \
				b.as.int_ = a.as_int() op b.as_int();                                              \
			} else if (b.is_float()) {                                                             \
				b.as.float_ = a.as_float() op b.as_float();                                        \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		pop();                                                                                     \
	} while (false);

#ifdef SNAP_DEBUG_RUNTIME
void print_stack(Value stack[VM::StackMaxSize], size_t sp) {
	printf("(%zu) [", sp);
	for (Value* v = stack; v < stack + sp; v++) {
		printf("%s", v->name_str().c_str());
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
		
		case Op::div: {
			Value& a = m_stack[sp - 1];
			Value& b = m_stack[sp - 2];

			double a_flt, b_flt;

			if (try_float_cast(a, &a_flt) && try_float_cast(a, &b_flt)) {
				if (b_flt == 0) {
					// TODO error
				} else {
					b.as.float_ = a_flt / b_flt;
				}
			} else {
				// TODO error
			}

			pop();
			break;
		}
		
		case Op::mod: {
			Value& a = m_stack[sp - 1];
			Value& b = m_stack[sp - 2];

			double a_flt, b_flt;

			if (a.is_int() && b.is_int()) {
				b.as.int_ = a.as_int() % b.as_int();
			} else if (try_float_cast(a, &a_flt) && try_float_cast(a, &b_flt)) {
				b.as.float_ = fmod(a_flt, b_flt);
			}

			pop();
			break;
		}
		
		case Op::eq: {
			Value& a = m_stack[sp - 1];
			Value& b = m_stack[sp - 2];

			push(Value(Value::are_equal(a, b)));
			pop();
			break;
		}

		case Op::get_var: {
			u8 idx = NEXT_BYTE();
			push(GET_VAR(idx));
			break;
		}

		case Op::set_var: {
			u8 idx = NEXT_BYTE();
			SET_VAR(idx, peek(0));
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