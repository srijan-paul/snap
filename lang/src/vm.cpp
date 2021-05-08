#include "common.hpp"
#include "std/vystrlib.hpp"
#include "str_format.hpp"
#include "value.hpp"
#include <cmath>
#include <cstddef>
#include <std/base.hpp>
#include <std/primitives/vy_string.hpp>
#include <vm.hpp>

#if defined(VYSE_DEBUG_RUNTIME) || defined(VYSE_DEBUG_DISASSEMBLY)
#include <cstdio>
#include <debug.hpp>
#endif

#define ERROR(...)		 runtime_error(kt::format_str(__VA_ARGS__))
#define INDEX_ERROR(v) ERROR("Attempt to index a '{}' value.", VYSE_TYPE_CSTR(v))
#define CURRENT_LINE() (m_current_block->lines[ip - 1])

#define CHECK_TYPE(v, typ, ...)                                                                    \
	if (!VYSE_CHECK_TT(v, typ)) return ERROR(__VA_ARGS__)

#define FETCH()			(m_current_block->code[ip++])
#define NEXT_BYTE() (static_cast<u8>(m_current_block->code[ip++]))
#define FETCH_SHORT()                                                                              \
	(ip += 2, (u16)((static_cast<u8>(m_current_block->code[ip - 2]) << 8) |                          \
									static_cast<u8>(m_current_block->code[ip - 1])))
#define READ_VALUE()					(m_current_block->constant_pool[NEXT_BYTE()])
#define GET_VAR(index)				(m_current_frame->base[index])
#define SET_VAR(index, value) (m_current_frame->base[index] = value)

// PEEK(1) fetches the topmost value in the stack.
#define PEEK(depth) m_stack.top[-depth]
#define POP()				(m_stack.pop())
#define DISCARD()		(--m_stack.top)
#define POPN(n)			(m_stack.top -= n)
#define PUSH(value) m_stack.push(value)

namespace vyse {

using Op = Opcode;
using VT = ValueType;
using OT = ObjType;

#define IS_VAL_FALSY(v)	 ((VYSE_IS_BOOL(v) and !(VYSE_AS_BOOL(v))) or VYSE_IS_NIL(v))
#define IS_VAL_TRUTHY(v) (!IS_VAL_FALSY(v))

#define UNOP_ERROR(op, v) ERROR("Cannot use operator '{}' on type '{}'.", op, VYSE_TYPE_CSTR(v))

#define CMP_OP(op)                                                                                 \
	do {                                                                                             \
		Value b = m_stack.pop();                                                                       \
		Value a = m_stack.pop();                                                                       \
                                                                                                   \
		if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {                                                       \
			PUSH(VYSE_BOOL(VYSE_AS_NUM(a) op VYSE_AS_NUM(b)));                                           \
		} else {                                                                                       \
			return binop_error(#op, b, a);                                                               \
		}                                                                                              \
	} while (false);

#define BINOP(op)                                                                                  \
	do {                                                                                             \
		Value& a = PEEK(1);                                                                            \
		Value& b = PEEK(2);                                                                            \
                                                                                                   \
		if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {                                                       \
			VYSE_SET_NUM(b, VYSE_AS_NUM(b) op VYSE_AS_NUM(a));                                           \
			DISCARD();                                                                                   \
		} else {                                                                                       \
			return binop_error(#op, b, a);                                                               \
		}                                                                                              \
	} while (false);

#define BIT_BINOP(op)                                                                              \
	Value& b = PEEK(1);                                                                              \
	Value& a = PEEK(2);                                                                              \
                                                                                                   \
	if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {                                                         \
		VYSE_SET_NUM(a, VYSE_CAST_INT(a) op VYSE_CAST_INT(b));                                         \
		DISCARD();                                                                                     \
	} else {                                                                                         \
		return binop_error(#op, a, b);                                                                 \
	}

#ifdef VYSE_DEBUG_RUNTIME
void print_stack(Value* stack, size_t sp) {
	printf("(%zu)[ ", sp);
	for (Value* v = stack; v < stack + sp; v++) {
		print_value(*v);
		printf(" ");
	}
	printf("]\n");
}
#endif

ExitCode VM::run() {

	while (true) {
		const Op op = FETCH();
#ifdef VYSE_DEBUG_RUNTIME
		disassemble_instr(*m_current_block, op, ip - 1);
#endif

		switch (op) {
		case Op::load_const: PUSH(READ_VALUE()); break;
		case Op::load_nil: PUSH(VYSE_NIL); break;

		case Op::pop: m_stack.pop(); break;
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

			if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {
				if (VYSE_AS_NUM(b) == 0) {
					return runtime_error("Attempt to divide by 0.\n");
				}
				VYSE_SET_NUM(b, VYSE_AS_NUM(b) / VYSE_AS_NUM(a));
				DISCARD();
			} else {
				return binop_error("/", b, a);
			}
			break;
		}

		case Op::exp: {
			Value& base = PEEK(2);
			Value& power = PEEK(1);
			if (VYSE_IS_NUM(base) and VYSE_IS_NUM(power)) {
				VYSE_SET_NUM(base, pow(VYSE_AS_NUM(base), VYSE_AS_NUM(power)));
				DISCARD();
			} else {
				return binop_error("**", base, power);
			}
			break;
		}

		case Op::mod: {
			Value& a = PEEK(1);
			Value& b = PEEK(2);

			if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {
				VYSE_SET_NUM(b, fmod(VYSE_AS_NUM(b), VYSE_AS_NUM(a)));
			} else {
				return binop_error("%", b, a);
			}

			POP();
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

		case Op::bxor: {
			BIT_BINOP(^);
			break;
		}

		case Op::bor: {
			BIT_BINOP(|);
			break;
		}

		case Op::eq: {
			Value a = m_stack.pop();
			Value b = m_stack.pop();
			PUSH(VYSE_BOOL(a == b));
			break;
		}

		case Op::neq: {
			Value a = POP();
			Value b = POP();
			PUSH(VYSE_BOOL(a != b));
			break;
		}

		case Op::negate: {
			Value& operand = PEEK(1);
			if (VYSE_IS_NUM(operand))
				VYSE_SET_NUM(operand, -VYSE_AS_NUM(operand));
			else
				UNOP_ERROR("-", operand);
			break;
		}

		case Op::lnot: {
			Value a = POP();
			PUSH(VYSE_BOOL(IS_VAL_FALSY(a)));
			break;
		}

		case Op::len: {
			Value v = POP();
			if (VYSE_IS_TABLE(v)) {
				PUSH(VYSE_NUM(VYSE_AS_TABLE(v)->length()));
			} else if (VYSE_IS_STRING(v)) {
				PUSH(VYSE_NUM(VYSE_AS_STRING(v)->m_length));
			} else {
				return ERROR("Attempt to get length of a {} value", VYSE_TYPE_CSTR(v));
			}
			break;
		}

		case Op::bnot: {
			if (VYSE_IS_NUM(PEEK(1))) {
				VYSE_SET_NUM(PEEK(1), ~s64(VYSE_AS_NUM(PEEK(1))));
			} else {
				return ERROR("Cannot use operator '~' on value of type '{}'", VYSE_TYPE_CSTR(PEEK(1)));
			}
			break;
		}

		case Op::jmp_if_true_or_pop: {
			Value& top = PEEK(1);
			if (IS_VAL_TRUTHY(top)) {
				ip += FETCH_SHORT();
			} else {
				ip += 2;
				POP();
			}
			break;
		}

		case Op::jmp_if_false_or_pop: {
			Value& top = PEEK(1);
			if (IS_VAL_FALSY(top)) {
				ip += FETCH_SHORT();
			} else {
				ip += 2;
				POP();
			}
			break;
		}

		case Op::jmp: {
			ip += FETCH_SHORT();
			break;
		}

		case Op::jmp_back: {
			u16 dist = FETCH_SHORT();
			ip -= dist;
			break;
		}

		// In a for loop, the variables are to be set up in the
		// stack as such: [counter, limit, step, i]
		// Here, 'i' is the user exposed counter. The user is free to modify
		// this variable, however the 'for_loop' instruction at the end
		// of a loop's body will change it back to `counter + limit`.
		// counter = counter - 1;
		// i = counter;
		// jump to to coressponding for_loop opcode;
		// make some type checks;
		case Op::for_prep: {
			Value& counter = PEEK(3);
			CHECK_TYPE(counter, VT::Number, "'for' variable not a number.");
			CHECK_TYPE(PEEK(2), VT::Number, "'for' limit not a number.");
			const Value& step = PEEK(1);
			CHECK_TYPE(step, VT::Number, "'for' step not a number.");
			VYSE_SET_NUM(counter, VYSE_AS_NUM(counter) - VYSE_AS_NUM(step));
			PUSH(counter); // load the user exposed loop counter (i).
			ip += FETCH_SHORT();
			break;
		}

		// counter += step
		// i = counter
		// if (counter < limit) jump to start;
		case Op::for_loop: {
			Value& counter = PEEK(4);
			const Value& limit = PEEK(3);
			const Value& step = PEEK(2);

			const number nstep = VYSE_AS_NUM(step);
			// update loop counter.
			VYSE_SET_NUM(counter, VYSE_AS_NUM(counter) + nstep);
			// update user exposed loop variable (i)
			PEEK(1) = counter;

			if (nstep >= 0) {
				if (VYSE_AS_NUM(counter) < VYSE_AS_NUM(limit)) {
					ip -= FETCH_SHORT();
					break;
				} // else fall to 'ip += 2'
			} else if (VYSE_AS_NUM(counter) >= VYSE_AS_NUM(limit)) {
				ip -= FETCH_SHORT();
				break;
			}

			ip += 2;
			break;
		}

		case Op::get_var: {
			u8 idx = NEXT_BYTE();
			PUSH(GET_VAR(idx));
			break;
		}

		case Op::set_var: {
			u8 idx = NEXT_BYTE();
			SET_VAR(idx, POP());
			break;
		}

		case Op::set_upval: {
			u8 idx = NEXT_BYTE();
			VYSE_ASSERT(m_current_frame->func->tag == OT::closure, "enclosing frame a CClosure!");
			Closure* cl = static_cast<Closure*>(m_current_frame->func);
			*cl->get_upval(idx)->m_value = POP();
			break;
		}

		case Op::get_upval: {
			u8 idx = NEXT_BYTE();
			VYSE_ASSERT(m_current_frame->func->tag == OT::closure, "enclosing frame a CClosure!");
			Closure* cl = static_cast<Closure*>(m_current_frame->func);
			PUSH(*cl->get_upval(idx)->m_value);
			break;
		}

		case Op::set_global: {
			Value name = READ_VALUE();
			VYSE_ASSERT(VYSE_IS_STRING(name), "Variable name not a string.");
			set_global(VYSE_AS_STRING(name), POP());
			break;
		}

		case Op::get_global: {
			Value name = READ_VALUE();
			VYSE_ASSERT_OT(name, OT::string);
			Value value = get_global(VYSE_AS_STRING(name));
			if (VYSE_IS_UNDEFINED(value)) {
				return ERROR("Undefined variable '{}'.", VYSE_AS_STRING(name)->c_str());
			}
			PUSH(value);
			break;
		}

		case Op::close_upval: {
			close_upvalues_upto(m_stack.top - 1);
			POP();
			break;
		}

		case Op::concat: {
			Value& a = PEEK(2);
			Value b = POP();

			if (!(VYSE_IS_STRING(a) and VYSE_IS_STRING(b))) {
				return binop_error("..", a, b);
			} else {
				auto left = VYSE_AS_STRING(a);
				auto right = VYSE_AS_STRING(b);

				// The second string has been popped
				// off the stack and might not be reachable.
				// by the GC. The allocation of the concatenated
				// string might trigger a GC cycle.
				gc_protect(right);
				a = concatenate(left, right);
				gc_unprotect(right);
			}
			break;
		}

		case Op::new_table: {
			PUSH(VYSE_OBJECT(&make<Table>()));
			break;
		}

		case Op::table_add_field: {
			Value value = POP();
			Value key = POP();

			Value vtable = PEEK(1);
			VYSE_AS_TABLE(vtable)->set(key, value);
			break;
		}

		// table[key] = value
		case Op::index_set: {
			Value value = POP();
			Value key = POP();
			if (VYSE_IS_NIL(key)) return ERROR("Table key cannot be nil.");

			Value& tvalue = PEEK(1);
			if (VYSE_IS_TABLE(tvalue)) {
				VYSE_AS_TABLE(tvalue)->set(key, value);
				m_stack.top[-1] = value; // assignment returns it's RHS.
			} else {
				return ERROR("Attempt to index a {} value.", VYSE_TYPE_CSTR(tvalue));
			}
			break;
		}

		// table.key = value
		case Op::table_set: {
			const Value& key = READ_VALUE();
			if (VYSE_IS_NIL(key)) return ERROR("Table key cannot be nil.");
			Value value = POP();
			Value& tvalue = PEEK(1);
			if (VYSE_IS_TABLE(tvalue)) {
				VYSE_AS_TABLE(tvalue)->set(key, value);
				m_stack.top[-1] = value; // assignment returns it's RHS
			} else {
				return INDEX_ERROR(tvalue);
			}
			break;
		}

		// table.key
		case Op::table_get: {
			// TOS = as_table(TOS)->get(READ_VAL())
			Value tvalue = PEEK(1);
			if (VYSE_IS_TABLE(tvalue)) {
				m_stack.top[-1] = VYSE_AS_TABLE(tvalue)->get(READ_VALUE());
			} else {
				return INDEX_ERROR(tvalue);
			}
			break;
		}

		// table.key
		case Op::table_get_no_pop: {
			// push((TOS)->get(READ_VAL()))
			Value tval = PEEK(1);
			if (VYSE_IS_TABLE(tval)) {
				PUSH(VYSE_AS_TABLE(tval)->get(READ_VALUE()));
			} else {
				return INDEX_ERROR(tval);
			}
			break;
		}

		// table_or_string_or_array[key]
		case Op::index: {
			Value key = POP();
			Value& tvalue = PEEK(1);

			if (VYSE_IS_TABLE(tvalue)) {
				tvalue = VYSE_AS_TABLE(tvalue)->get(key);
			} else if (VYSE_IS_STRING(tvalue)) {
				CHECK_TYPE(key, VT::Number, "string index must be a number (got %s)", VYSE_TYPE_CSTR(key));
				const String* str = VYSE_AS_STRING(tvalue);
				const u64 index = VYSE_AS_NUM(key);
				if (index < 0 or index >= str->m_length) {
					return ERROR("string index out of range.");
				}
				VYSE_SET_OBJECT(tvalue, char_at(str, index));
			} else {
				return INDEX_ERROR(tvalue);
			}
			break;
		}

		// table_or_array[key]
		case Op::index_no_pop: {
			Value& vtable = PEEK(2);
			const Value& key = PEEK(1);
			if (VYSE_IS_TABLE(vtable)) {
				if (VYSE_IS_NIL(key)) return ERROR("Table key cannot be nil.");
				PUSH(VYSE_AS_TABLE(vtable)->get(key));
			} else {
				return INDEX_ERROR(vtable);
			}
			break;
		}

		case Op::pop_jmp_if_false: {
			ip += IS_VAL_FALSY(PEEK(1)) ? FETCH_SHORT() : 2;
			DISCARD();
			break;
		}

		// tbl <- POP()
		// PUSH(tbl[READ_VALUE()])
		// PUSH(tbl)
		case Op::prep_method_call: {
			Value vtable = PEEK(1);
			Value vkey = READ_VALUE();
			VYSE_ASSERT(VYSE_IS_STRING(vkey), "method name not a string.");

			if (VYSE_IS_NIL(vtable)) return INDEX_ERROR(vtable);
			if (VYSE_IS_TABLE(vtable)) {
				m_stack.top[-1] = VYSE_AS_TABLE(vtable)->get(vkey);
			} else {
				m_stack.top[-1] = index_primitive(vtable, vkey);
			}
			PUSH(vtable);

			break;
		}

		case Op::call_func: {
			u8 argc = NEXT_BYTE();
			Value value = PEEK(argc - 1);
			if (!call(value, argc)) return ExitCode::RuntimeError;
			break;
		}

		case Op::return_val: {
			Value result = POP();
			close_upvalues_upto(m_current_frame->base);
			m_stack.top = m_current_frame->base + 1;

			DISCARD();
			PUSH(result);

			m_frame_count--;
			if (m_frame_count == 0) {
				return_value = result;
				return ExitCode::Success;
			}

			m_current_frame = &m_frames[m_frame_count - 1];
			m_current_block = &static_cast<Closure*>(m_current_frame->func)->m_codeblock->block();
			ip = m_current_frame->ip;

			break;
		}

		case Op::make_func: {
			Value vcode = READ_VALUE();
			VYSE_ASSERT_OT(vcode, OT::codeblock);
			u32 num_upvals = NEXT_BYTE();
			Closure* func = &make<Closure>(VYSE_AS_PROTO(vcode), num_upvals);

			PUSH(VYSE_OBJECT(func));

			for (u8 i = 0; i < num_upvals; ++i) {
				bool is_local = NEXT_BYTE();
				u8 index = NEXT_BYTE();

				if (is_local) {
					func->set_upval(i, capture_upvalue(m_current_frame->base + index));
				} else {
					Closure* cl = static_cast<Closure*>(m_current_frame->func);
					func->set_upval(i, cl->get_upval(index));
				}
			}

			break;
		}

		default: {
			VYSE_ERROR("Impossible opcode.");
			return ExitCode::RuntimeError;
		}
		}
#ifdef VYSE_DEBUG_RUNTIME
		// printf("[stack max: %zu]\t", m_stack.size);
		print_stack(m_stack.values, m_stack.top - m_stack.values);
		printf("\n");
#endif
	}

	return ExitCode::Success;
}

Value VM::concatenate(const String* left, const String* right) {
	size_t length = left->len() + right->len();

	char* buf = new char[length + 1];
	buf[length] = '\0';

	std::memcpy(buf, left->c_str(), left->len());
	std::memcpy(buf + left->len(), right->c_str(), right->len());

	size_t hash = hash_cstring(buf, length);
	String* interned = interned_strings.find_string(buf, length, hash);

	if (interned == nullptr) {
		String* res = &make<String>(buf, length, hash);
		Value vresult = VYSE_OBJECT(res);
		interned_strings.set(vresult, VYSE_BOOL(true));
		return vresult;
	} else {
		delete[] buf;
		return VYSE_OBJECT(interned);
	}
}

Value VM::get_global(String* name) const {
	auto search = m_global_vars.find(name);
	if (search == m_global_vars.end()) return VYSE_UNDEF;
	return search->second;
}

Value VM::get_global(const char* name) {
	String& sname = make_string(name, strlen(name));
	return get_global(&sname);
}

void VM::set_global(String* name, Value value) {
	m_global_vars[name] = value;
}

void VM::set_global(const char* name, Value value) {
	String& sname = make_string(name, strlen(name));
	m_global_vars[&sname] = value;
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
	bool ok = init();
	if (!ok) {
		m_has_error = true;
		return ExitCode::CompileError;
	}
	return run();
}

bool VM::init() {
	if (m_source == nullptr) {
		ERROR("No code to run.");
		return false;
	}

	Compiler compiler{this, m_source};
	m_compiler = &compiler;

	CodeBlock* code = m_compiler->compile();
	// If the compilation failed, return false
	// and signal a compile time error.
	if (!compiler.ok()) return false;

	// There are no reachable references to [code]
	// when we allocate `func`. Since allocating a func
	// can trigger a garbage collection cycle, we protect
	// the code block.
	gc_protect(code);
	Closure* func = &make<Closure>(code, 0);

	// Once the function has been made, [code] can
	// be reached via `func->m_codeblock`, so we can
	// unprotect it.
	gc_unprotect(code);

	PUSH(VYSE_OBJECT(func));
	callfunc(func, 0);

#ifdef VYSE_DEBUG_DISASSEMBLY
	disassemble_block(func->name()->c_str(), *m_current_block);
	printf("\n");
#endif
	m_compiler = nullptr;
	return true;
}

ExitCode VM::runcode(const std::string& code) {
	m_source = &code;
	return interpret();
}

void VM::add_stdlib_object(const char* name, Obj* o) {
	auto vglobal = VYSE_OBJECT(o);
	// setting a global variable may trigger a garbage collection
	// cycle (when allocating the [name] as vyse::String). At that
	// point, vprint is only reachable on the C stack, so we protect
	// it by pushing it on to the VM stack.
	PUSH(vglobal);
	set_global(name, vglobal);
	DISCARD();
}

void VM::load_stdlib() {
	add_stdlib_object("print", &make<CClosure>(stdlib::print));
	add_stdlib_object("setproto", &make<CClosure>(stdlib::setproto));
	add_stdlib_object("byte", &make<CClosure>(stdlib::byte));

	load_primitives();
}

void VM::load_primitives() {
	primitive_protos.string = &make<Table>();
	set_global("String", VYSE_OBJECT(primitive_protos.string));

	primitive_protos.number = &make<Table>();
	set_global("Number", VYSE_OBJECT(primitive_protos.number));

	primitive_protos.boolean = &make<Table>();
	set_global("Bool", VYSE_OBJECT(primitive_protos.boolean));

	stdlib::primitives::load_string_proto(*this);
}

using OT = ObjType;

Upvalue* VM::capture_upvalue(Value* slot) {
	// start at the head of the linked list
	Upvalue* current = m_open_upvals;
	Upvalue* prev = nullptr;

	// keep going until we reach a slot whose
	// depth is lower than what we've been looking
	// for, or until we reach the end of the list.
	while (current != nullptr and current->m_value < slot) {
		prev = current;
		current = current->next_upval;
	}

	// We've found an upvalue that was
	// already capturing a value at this stack
	// slot, so we reuse the existing upvalue
	if (current != nullptr and current->m_value == slot) return current;

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
	while (m_open_upvals != nullptr and m_open_upvals->m_value >= last) {
		Upvalue* current = m_open_upvals;
		// these two lines are the last rites of an
		// upvalue, closing it.
		current->closed = *current->m_value;
		current->m_value = &current->closed;
		m_open_upvals = current->next_upval;
	}
}

bool VM::call(Value value, u8 argc) {
	if (!VYSE_CHECK_TT(value, VT::Object))
		ERROR("Attempt to call a {} value.", VYSE_TYPE_CSTR(value));

	switch (VYSE_AS_OBJECT(value)->tag) {
	case OT::closure: return callfunc(VYSE_AS_CLOSURE(value), argc);
	case OT::c_closure: return call_cclosure(VYSE_AS_CCLOSURE(value), argc);
	default: ERROR("Attempt to call a {} value.", VYSE_TYPE_CSTR(value)); return false;
	}

	return false;
}

void VM::push_callframe(Obj* callable, int argc) {
	VYSE_ASSERT(callable->tag == OT::c_closure or callable->tag == OT::closure,
							"Non callable callframe pushed.");

	// Save the current instruction pointer
	// in the call frame so we can resume
	// execution when the function returns.
	m_current_frame->ip = ip;
	// prepare the next call frame
	m_current_frame = &m_frames[m_frame_count++];
	m_current_frame->func = callable;
	m_current_frame->base = m_stack.top - argc - 1;

	// start from the first opcode
	m_current_frame->ip = ip = 0;

	if (callable->tag == OT::closure) {
		Closure* cl = static_cast<Closure*>(callable);
		m_current_block = &cl->m_codeblock->block();
	}
}

void VM::pop_callframe() {
	m_frame_count--;

	/// If we are in the top level script,
	/// then there is no older call frame.
	if (m_frame_count == 0) return;

	m_current_frame = &m_frames[m_frame_count - 1];

	/// restore the instruction pointer to continue
	/// from where we left off.
	ip = m_current_frame->ip;

	Obj* callable = m_current_frame->func;
	if (callable->tag == OT::closure) {
		Closure* cl = static_cast<Closure*>(callable);
		m_current_block = &cl->m_codeblock->block();
	}
}

bool VM::callfunc(Closure* func, int argc) {
	int extra = argc - func->m_codeblock->param_count();

	// make sure there is enough room in the stack
	// for this function call.
	ensure_slots(func->m_codeblock->stack_size());

	// extra arguments are ignored and
	// arguments that aren't provded are replaced with nil.
	if (extra < 0) {
		while (extra < 0) {
			PUSH(VYSE_NIL);
			argc++;
			extra++;
		}
	} else {
		while (extra > 0) {
			DISCARD();
			argc--;
			extra--;
		}
	}

	push_callframe(func, argc);
	return true;
}

bool VM::call_cclosure(CClosure* cclosure, int argc) {
	push_callframe(cclosure, argc);
	CFunction c_func = cclosure->cfunc();
	Value ret = c_func(*this, argc);
	pop_callframe();

	m_stack.popn(argc);
	m_stack.top[-1] = ret;

	return !m_has_error;
}

Value VM::index_primitive(const Value& value, const Value& key) {
	switch (VYSE_GET_TT(value)) {
	case VT::Bool: return primitive_protos.boolean->get(key);
	case VT::Number: return primitive_protos.number->get(key);
	default: {
		if (VYSE_IS_STRING(value)) {
			return primitive_protos.string->get(key);
		}
		return VYSE_NIL;
	}
	}
}

// String operation helpers.

String* VM::char_at(const String* string, u64 index) {
	char* charbuf = new char[2]{string->at(index), '\0'};
	return &take_string(charbuf, 1);
}

String& VM::take_string(char* buf, size_t len) {
	size_t hash = hash_cstring(buf, len);

	// look for an existing interened copy of the string.
	String* interend = interned_strings.find_string(buf, len, hash);
	if (interend != nullptr) {
		// we now 'own' the string, so we are free to
		// get rid of this buffer if we don't need it.
		delete[] buf;
		return *interend;
	}

	String& string = make<String>(buf, len, hash);
	interned_strings.set(VYSE_OBJECT(&string), VYSE_BOOL(true));
	return string;
}

String& VM::make_string(const char* chars, size_t length) {
	size_t hash = hash_cstring(chars, length);

	// If an identical string has already been created, then
	// return a reference to the existing string instead.
	String* interned = interned_strings.find_string(chars, length, hash);
	if (interned != nullptr) return *interned;

	String* string = &make<String>(chars, length, hash);
	interned_strings.set(VYSE_OBJECT(string), VYSE_BOOL(true));

	return *string;
}

void VM::ensure_slots(size_t slots_needed) {
	std::ptrdiff_t num_values = m_stack.top - m_stack.values;
	size_t num_free_slots = m_stack.size - num_values;

	// Requested number of slots is already available.
	if (num_free_slots > slots_needed) return;

	m_stack.size += (slots_needed - num_free_slots) + 1;
	Value* old_stack = m_stack.values;
	m_stack.values = static_cast<Value*>(realloc(m_stack.values, m_stack.size * sizeof(Value)));

	// Now that the stack has moved in memory, the CallFrames and the
	// Upvalue chain still contain dangling pointers to the old stack,
	// so we update those to the same relative distance from the new
	// stack's base address.
	for (int i = m_frame_count - 1; i >= 0; --i) {
		CallFrame& cf = m_frames[i];
		cf.base = m_stack.values + (cf.base - old_stack);
	}

	for (Upvalue* upval = m_open_upvals; upval != nullptr; upval = upval->next_upval) {
		upval->m_value = (upval->m_value - old_stack) + m_stack.values;
	}

	m_stack.top = m_stack.values + num_values;
}

// 	-- Garbage collection --

size_t VM::collect_garbage() {
	if (can_collect) {
		m_gc.mark();
		m_gc.trace();
		return m_gc.sweep();
	}
	return 0;
}

// -- Error reporting --

ExitCode VM::binop_error(const char* opstr, const Value& a, const Value& b) {
	return ERROR("Cannot use operator '{}' on operands of type '{}' and '{}'.", opstr,
							 VYSE_TYPE_CSTR(a), VYSE_TYPE_CSTR(b));
}

ExitCode VM::runtime_error(const std::string& message) {
	m_has_error = true;

	std::string error_str =
			(m_current_frame->is_cclosure())
					? kt::format_str("{}\nstack trace:\n", message)
					: kt::format_str("[line {}]: {}\nstack trace:\n", CURRENT_LINE(), message);

	for (int i = m_frame_count - 1; i >= 0; --i) {
		const CallFrame& frame = m_frames[i];

		/// TODO: Handle CFunction strack traces.
		if (frame.is_cclosure()) continue;

		const Closure& func = *static_cast<Closure*>(frame.func);

		const Block& block = func.m_codeblock->block();
		VYSE_ASSERT(frame.ip >= 0 and frame.ip < block.lines.size(),
								"IP not in range for std::vector<u32> block.lines.");

		int line = block.lines[frame.ip];
		if (i == 0) {
			error_str += kt::format_str("\t[line {}] in {}", line, func.name_cstr());
		} else {
			error_str += kt::format_str("\t[line {}] in function {}.\n", line, func.name_cstr());
		}
	}

	on_error(*this, error_str);
	return ExitCode::RuntimeError;
}

// The default behavior on an error is to simply
// print it to the stderr.
void default_error_fn([[maybe_unused]] const VM& vm, std::string& err_msg) {
	fprintf(stderr, "%s\n", err_msg.c_str());
}

/// TODO: The user might need some objects even after the VM
/// has been destructed. Add support for this.
VM::~VM() {
	if (m_gc.m_objects == nullptr) return;
	for (Obj* object = m_gc.m_objects; object != nullptr;) {
		Obj* next = object->next;
		delete object;
		object = next;
	}
}

} // namespace vyse
