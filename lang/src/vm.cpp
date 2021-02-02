#include <cmath>
#include <common.hpp>
#include <compiler.hpp>
#include <cstdarg>
#include <cstdio>
#include <stdio.h>
#include <value.hpp>
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
#define GET_VAR(index)		  (m_current_frame->base[index])
#define SET_VAR(index, value) (m_current_frame->base[index] = value)

#define PEEK(depth) sp[-depth]

namespace snap {

using Op = Opcode;
using VT = ValueType;

VM::VM(const std::string* src) : m_source{src} {
}

#define IS_VAL_FALSY(v)	 ((SNAP_IS_BOOL(v) and !(SNAP_AS_BOOL(v))) || SNAP_IS_NIL(v))
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
		if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {                                                   \
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
		if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {                                                   \
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
	if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {                                                       \
		SNAP_SET_NUM(a, SNAP_CAST_INT(a) op SNAP_CAST_INT(b));                                     \
		pop();                                                                                     \
	} else {                                                                                       \
		BINOP_ERROR(#op, a, b);                                                                    \
	}

#ifdef SNAP_DEBUG_RUNTIME
void print_stack(Value* stack, size_t sp) {
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
		case Op::load_nil: push(SNAP_NIL_VAL); break;

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

			if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {
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

			if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {
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

		case Op::set_upval: {
			u8 idx = NEXT_BYTE();
			*m_current_frame->func->upvals[idx]->value = peek(0);
			break;
		}

		case Op::get_upval: {
			u8 idx = NEXT_BYTE();
			push(*m_current_frame->func->upvals[idx]->value);
			break;
		}

		case Op::close_upval: {
			close_upvalues_upto(sp - 1);
			pop();
			break;
		}

		case Op::concat: {
			Value& a = PEEK(2);
			Value& b = PEEK(1);

			if (!(SNAP_IS_STRING(a) and SNAP_IS_STRING(b))) {
				return binop_error("..", a, b);
			} else {
				String* s = &make<String>(SNAP_AS_STRING(a), SNAP_AS_STRING(b));
				SNAP_SET_OBJECT(a, s);
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

		case Op::call_func: {
			u8 argc = NEXT_BYTE();
			Value value = PEEK(argc - 1);
			if (!call(value, argc)) return ExitCode::RuntimeError;
			break;
		}

		case Op::return_val: {
			Value result = pop();
			close_upvalues_upto(m_current_frame->base);
			sp = m_current_frame->base + 1;

			pop();
			push(result);

			m_frame_count--;
			if (m_frame_count == 0) {
				return_value = result;
				return ExitCode::Success;
			}

			m_current_frame = &m_frames[m_frame_count - 1];
			m_current_block = &m_current_frame->func->proto->m_block;
			ip = m_current_frame->ip;

			break;
		}
		case Op::make_func: {
			Prototype* proto = static_cast<Prototype*>(SNAP_AS_OBJECT(READ_VALUE()));
			Function* func = &make<Function>(proto);

			push(SNAP_OBJECT_VAL(func));

			u8 num_upvals = NEXT_BYTE();
			func->set_num_upvals(num_upvals);

			for (u8 i = 0; i < num_upvals; ++i) {
				bool is_local = NEXT_BYTE();
				u8 index = NEXT_BYTE();

				if (is_local) {
					func->upvals[i] = (capture_upvalue(m_current_frame->base + index));
				} else {
					func->upvals[i] = (m_current_frame->func->upvals[index]);
				}
			}

			break;
		}
		default: {
			std::cout << "not implemented " << int(op) << " yet" << std::endl;
			return ExitCode::RuntimeError;
		}
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
	Compiler compiler{this, m_source};
	m_compiler = &compiler;

	Prototype* proto = m_compiler->compile();
	push(SNAP_OBJECT_VAL(proto));
	Function* func = &make<Function>(proto);
	pop();
	push(SNAP_OBJECT_VAL(func));
	callfunc(func, 0);

#ifdef SNAP_DEBUG_DISASSEMBLY
	disassemble_block(func->proto->name->chars, *m_current_block);
	printf("\n");
#endif

	m_compiler = nullptr;
	return true;
}

using OT = ObjType;

Upvalue* VM::capture_upvalue(Value* slot) {
	// start at the head of the linked list
	Upvalue* current = m_open_upvals;
	Upvalue* prev = nullptr;

	// keep going until we reach a slot whose
	// depth is lower than what we've been looking
	// for, or until we reach the end of the list.
	while (current != nullptr and current->value < slot) {
		prev = current;
		current = current->next_upval;
	}

	// We've found an upvalue that was
	// already capturing a value at this stack
	// slot, so we reuse the existing upvalue
	if (current != nullptr and current->value == slot) return current;

	// We've reached a node in the list where the previous node is above the
	// slot we wanted to capture, but the current node is deeper.
	// Meaning `slot` points to a new value that hasn't been captured before.
	// So we add it between `prev` and `current`.

	Upvalue* upval = &make<Upvalue>(slot);
	upval->next_upval = current;

	// prev is null when there are no upvalues.
	if (prev == nullptr) {
		m_open_upvals = upval;
	} else {
		prev->next_upval = upval;
	}

	return upval;
}

void VM::close_upvalues_upto(Value* last) {
	Upvalue* current = m_open_upvals;

	while (current != nullptr and current->value >= last) {

		// these two lines are the last rites of an
		// upvalue, closing it.
		current->closed = *current->value;
		current->value = &current->closed;
		current = current->next_upval;
	}
}

bool VM::call(Value value, u8 argc) {
	switch (SNAP_GET_TT(value)) {
	case VT::Object: {
		switch (SNAP_AS_OBJECT(value)->tag) {
		case OT::func: return callfunc(SNAP_AS_FUNCTION(value), argc);
		default: runtime_error("Attempt to call a %s value.", SNAP_TYPE_CSTR(value)); return false;
		}
		break;
	}
	default: runtime_error("Attempt to call a %s value.", SNAP_TYPE_CSTR(value));
	}

	return false;
}

bool VM::callfunc(Function* func, int argc) {
	int extra = argc - func->proto->num_params;

	// extra arguments are ignored and
	// arguments that aren't provded are replaced with nil.
	if (extra < 0) {
		while (extra < 0) {
			push(SNAP_NIL_VAL);
			argc++;
			extra++;
		}
	} else {
		while (extra > 0) {
			pop();
			argc--;
			extra--;
		}
	}

	m_current_frame->ip = ip;
	m_current_frame = &m_frames[m_frame_count++];
	m_current_frame->func = func;
	ip = m_current_frame->ip = 0;
	m_current_frame->base = sp - argc - 1;
	m_current_block = &func->proto->m_block;
	return true;
}

// 	-- Garbage collection --

void VM::collect_garbage() {
	// TODO
}

const Block* VM::block() {
	return m_current_block;
}

//  --- Error reporting ---

ExitCode VM::binop_error(const char* opstr, Value& a, Value& b) {
	return runtime_error("Cannot use operator '%s' on operands of type '%s' and '%s'.", opstr,
						 SNAP_TYPE_CSTR(a), SNAP_TYPE_CSTR(b));
}

ExitCode VM::runtime_error(const char* fstring...) const {
	va_list args;
	va_start(args, fstring);

	std::size_t bufsize = vsnprintf(nullptr, 0, fstring, args) + 1;
	std::unique_ptr<char> buf{new char[bufsize]};
	vsnprintf(buf.get(), bufsize, fstring, args);

	default_error_fn(*this, buf.get());
	va_end(args);
	return ExitCode::RuntimeError;
}

void default_error_fn(const VM& vm, const char* message) {
	fprintf(stderr, "%s", message);
	fputc('\n', stderr);
}

// TODO: FIX THIS MESS
VM::~VM() {
	// if (m_gc_objects == nullptr) return;
	// for (Obj* object = m_gc_objects; object != nullptr; ) {
	// 	print_value(SNAP_OBJECT_VAL(object));
	// 	printf("\n");
	// 	Obj* temp = object->next;
	// 	free_object(object);
	// 	object = temp;
	// }
}

} // namespace snap