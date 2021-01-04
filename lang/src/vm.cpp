#include "gc.hpp"
#include "value.hpp"
#include <cmath>
#include <common.hpp>
#include <compiler.hpp>
#include <cstdarg>
#include <cstdio>
#include <stdio.h>
#include <vm.hpp>

#if defined(SNAP_DEBUG_RUNTIME) || defined(SNAP_DEBUG_DISASSEMBLY)
#include <debug.hpp>
#endif

#define FETCH()		(m_current_block->code[ip++])
#define NEXT_BYTE() (static_cast<u8>(m_current_block->code[ip++]))
#define FETCH_SHORT()                                                                              \
	(ip += 2, (u16)((static_cast<u8>(m_current_block->code[ip - 2]) << 8) |                        \
					static_cast<u8>(m_current_block->code[ip - 1])))
#define READ_VALUE()		  (m_current_block->constant_pool[NEXT_BYTE()])
#define GET_VAR(index)		  (frame->slots[index])
#define SET_VAR(index, value) (frame->slots[index] = value)

#define PEEK(depth) sp[-depth]

namespace snap {
using Op = Opcode;
using VT = ValueType;

VM::VM(const std::string* src) : m_compiler(this, src), source{src} {
}

#define IS_VAL_FALSY(v)	 ((SNAP_IS_BOOL(v) && !(SNAP_AS_BOOL(v))) || SNAP_IS_NIL(v))
#define IS_VAL_TRUTHY(v) (!IS_VAL_FALSY(v))

#define BINOP_ERROR(op, v1, v2)                                                                    \
	runtime_error("Cannot use operator '%s' on operands of type '%s' and '%s'.", op,               \
				  SNAP_TYPE_CSTR(v1), SNAP_TYPE_CSTR(v2))
#define UNOP_ERROR(op, v)                                                                          \
	runtime_error("Cannot use operator '%s' on type '%s'.", op, SNAP_TYPE_CSTR(v))

#define CMP_OP(op)                                                                                 \
	do {                                                                                           \
		Value b = pop();                                                                           \
		Value a = pop();                                                                           \
                                                                                                   \
		if (SNAP_IS_NUM(a) && SNAP_IS_NUM(b)) {                                                    \
			push(SNAP_BOOL_VAL(SNAP_AS_NUM(a) op SNAP_AS_NUM(b)));                                 \
		} else {                                                                                   \
			binop_error(#op, b, a);                                                                \
		}                                                                                          \
	} while (false);

#define BINOP(op)                                                                                  \
	do {                                                                                           \
		Value& a = PEEK(1);                                                                        \
		Value& b = PEEK(2);                                                                        \
                                                                                                   \
		if (SNAP_IS_NUM(a) && SNAP_IS_NUM(b)) {                                                    \
			SNAP_SET_NUM(b, SNAP_AS_NUM(b) op SNAP_AS_NUM(a));                                     \
			pop();                                                                                 \
		} else {                                                                                   \
			BINOP_ERROR(#op, b, a);                                                                \
		}                                                                                          \
	} while (false);

#define BIT_BINOP(op)                                                                              \
	Value& b = PEEK(1);                                                                            \
	Value& a = PEEK(2);                                                                            \
                                                                                                   \
	if (SNAP_IS_NUM(a) && SNAP_IS_NUM(b)) {                                                        \
		SNAP_SET_NUM(a, SNAP_CAST_INT(a) op SNAP_CAST_INT(b));                                     \
		pop();                                                                                     \
	} else {                                                                                       \
		BINOP_ERROR(#op, a, b);                                                                    \
	}

#ifdef SNAP_DEBUG_RUNTIME
void print_stack(Value stack[VM::StackMaxSize], size_t sp) {
	printf("(%zu)[ ", sp);
	for (Value* v = stack; v < stack + sp; v++) {
		printf("%s", v->name_str().c_str());
		printf(" ");
	}
	printf("]\n");
}
#endif

ExitCode VM::run(bool run_till_end) {

	do {
		const Op op = FETCH();

#ifdef SNAP_DEBUG_RUNTIME
		disassemble_instr(*m_current_block, op, ip - 1);
#endif

		switch (op) {
		case Op::load_const: push(READ_VALUE()); break;

		case Op::pop: pop(); break;
		case Op::add: BINOP(+); break;
		case Op::sub: BINOP(-); break;
		case Op::mult: BINOP(*); break;

		case Op::gt: CMP_OP(>); break;
		case Op::lt: CMP_OP(<); break;
		case Op::gte: CMP_OP(>=); break;
		case Op::lte: CMP_OP(<=); break;

		case Op::div: {
			Value& a = PEEK(1);
			Value& b = PEEK(2);

			if (SNAP_IS_NUM(a) && SNAP_IS_NUM(b)) {
				if (SNAP_AS_NUM(b) == 0) {
					return runtime_error("Attempt to divide by 0.\n");
				}
				SNAP_SET_NUM(b, SNAP_AS_NUM(b) / SNAP_AS_NUM(a));
				pop();
			} else {
				BINOP_ERROR("/", b, a);
			}
			break;
		}

		case Op::mod: {
			Value& a = PEEK(1);
			Value& b = PEEK(2);

			if (SNAP_IS_NUM(a) && SNAP_IS_NUM(b)) {
				SNAP_SET_NUM(b, fmod(SNAP_AS_NUM(b), SNAP_AS_NUM(a)));
			} else {
				BINOP_ERROR("%", b, a);
			}

			pop();
			break;
		}

		case Op::lshift: {
			BIT_BINOP(<<);
			break;
		}

		case Op::rshift: {
			BIT_BINOP(>>);
			break;
		}

		case Op::band: {
			BIT_BINOP(&);
			break;
		}

		case Op::bor: {
			BIT_BINOP(|);
			break;
		}

		case Op::eq: {
			Value a = pop();
			Value b = pop();
			push(SNAP_BOOL_VAL(Value::are_equal(a, b)));
			break;
		}

		case Op::neq: {
			Value a = pop();
			Value b = pop();
			push(SNAP_BOOL_VAL(!Value::are_equal(a, b)));
			break;
		}

		case Op::negate: {
			Value& a = PEEK(1);
			if (SNAP_IS_NUM(a)) {
				SNAP_SET_NUM(a, -SNAP_AS_NUM(a));
			} else {
				UNOP_ERROR("-", a);
			}
			break;
		}

		case Op::lnot: {
			Value a = pop();
			push(SNAP_BOOL_VAL(IS_VAL_FALSY(a)));
			break;
		}

		case Op::jmp_if_true_or_pop: {
			Value& top = PEEK(1);
			if (IS_VAL_TRUTHY(top)) {
				ip += FETCH_SHORT();
			} else {
				ip += 2;
				pop();
			}
			break;
		}

		case Op::jmp_if_false_or_pop: {
			Value& top = PEEK(1);
			if (IS_VAL_FALSY(top)) {
				ip += FETCH_SHORT();
			} else {
				ip += 2;
				pop();
			}
			break;
		}

		case Op::jmp: {
			ip += FETCH_SHORT();
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

		case Op::concat: {
			Value& a = PEEK(2);
			Value& b = PEEK(1);

			if (!(SNAP_IS_STRING(a) && SNAP_IS_STRING(b))) {
				return binop_error("..", a, b);
			} else {
				String* s = String::concatenate(SNAP_AS_STRING(a), SNAP_AS_STRING(b));
				SNAP_SET_OBJECT(a, s);
				register_object(s);
			}
			pop();
			break;
		}

		case Op::pop_jmp_if_false: {
			Value& value = PEEK(1);
			ip += IS_VAL_FALSY(value) ? FETCH_SHORT() : 2;
			pop();
			break;
		}

		case Op::return_val: return ExitCode::Success;
		default: std::cout << "not implemented yet" << std::endl;
		}
#ifdef SNAP_DEBUG_RUNTIME
		print_stack(m_stack, sp - m_stack);
		printf("\n");
#endif
	} while (run_till_end);

	return ExitCode::Success;
}

#undef FETCH
#undef FETCH_SHORT
#undef NEXT_BYTE
#undef READ_VALUE
#undef GET_VAR
#undef SET_VAR
#undef BINOP
#undef BINOP_ERROR
#undef IS_VAL_TRUTHY
#undef CMP_OP
#undef PEEK

ExitCode VM::interpret() {
	init();
	return run();
}

bool VM::init() {
	Function* func = m_compiler.compile();
	std::cout << "compilation done\n";
	push(SNAP_OBJECT_VAL(func));
	callfunc(func, 0);

#ifdef SNAP_DEBUG_DISASSEMBLY
	disassemble_block(*m_current_block);
	printf("\n");
#endif

	return true;
}

bool VM::callfunc(Function* func, u8 argc) {
	frame = &frames[frame_count++];
	frame->func = func;
	frame->ip = 0;
	frame->slots = sp - argc - 1;
	m_current_block = &func->proto->m_block;
	return true;
}

// 	-- Garbage collection --
using OT = ObjType;

void VM::collect_garbage() {
	GC::mark_roots(*this);
	GC::trace_refs(*this);
	GC::sweep(*this);
}

//  --- Error reporting ---

ExitCode VM::binop_error(const char* opstr, Value& a, Value& b) {
	return runtime_error("Cannot use operator '%s' on operands of type '%s' and '%s'.", opstr,
						 SNAP_TYPE_CSTR(a), SNAP_TYPE_CSTR(b));
}

ExitCode VM::runtime_error(const char* fstring, ...) const {
	va_list args;
	va_start(args, fstring);

	std::size_t bufsize = vsnprintf(nullptr, 0, fstring, args) + 1;
	char* buf = new char[bufsize];
	vsnprintf(buf, bufsize, fstring, args);

	default_error_fn(*this, buf);
	free(buf);
	va_end(args);
	return ExitCode::RuntimeError;
}

void default_error_fn(const VM& vm, const char* message) {
	fprintf(stderr, "%s", message);
	fputc('\n', stderr);
}

VM::~VM() {
	if (gc_objects == nullptr) return;
	for (Obj* object = gc_objects; object != nullptr; object = object->next) {
		GC::free_object(object);
	}
}

} // namespace snap