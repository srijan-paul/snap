#include "common.hpp"
#include "str_format.hpp"
#include <cmath>
#include <std/base.hpp>
#include <vm.hpp>

#if defined(SNAP_DEBUG_RUNTIME) || defined(SNAP_DEBUG_DISASSEMBLY)
#include <cstdio>
#include <debug.hpp>
#endif

#define ERROR(...)		 runtime_error(kt::format_str(__VA_ARGS__))
#define INDEX_ERROR(v) ERROR("Attempt to index a '{}' value.", SNAP_TYPE_CSTR(v))
#define CURRENT_LINE() (m_current_block->lines[ip - 1])

#define FETCH()			(m_current_block->code[ip++])
#define NEXT_BYTE() (static_cast<u8>(m_current_block->code[ip++]))
#define FETCH_SHORT()                                                                              \
	(ip += 2, (u16)((static_cast<u8>(m_current_block->code[ip - 2]) << 8) |                          \
									static_cast<u8>(m_current_block->code[ip - 1])))
#define READ_VALUE()					(m_current_block->constant_pool[NEXT_BYTE()])
#define GET_VAR(index)				(m_current_frame->base[index])
#define SET_VAR(index, value) (m_current_frame->base[index] = value)

// PEEK(1) fetches the topmost value in the stack.
#define PEEK(depth) sp[-depth]
#define POP()				*(--sp)

namespace snap {

using Op = Opcode;
using VT = ValueType;
using OT = ObjType;

#define IS_VAL_FALSY(v)	 ((SNAP_IS_BOOL(v) and !(SNAP_AS_BOOL(v))) or SNAP_IS_NIL(v))
#define IS_VAL_TRUTHY(v) (!IS_VAL_FALSY(v))

#define UNOP_ERROR(op, v) ERROR("Cannot use operator '{}' on type '{}'.", op, SNAP_TYPE_CSTR(v))

#define CMP_OP(op)                                                                                 \
	do {                                                                                             \
		Value b = pop();                                                                               \
		Value a = pop();                                                                               \
                                                                                                   \
		if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {                                                       \
			push(SNAP_BOOL_VAL(SNAP_AS_NUM(a) op SNAP_AS_NUM(b)));                                       \
		} else {                                                                                       \
			return binop_error(#op, b, a);                                                               \
		}                                                                                              \
	} while (false);

#define BINOP(op)                                                                                  \
	do {                                                                                             \
		Value& a = PEEK(1);                                                                            \
		Value& b = PEEK(2);                                                                            \
                                                                                                   \
		if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {                                                       \
			SNAP_SET_NUM(b, SNAP_AS_NUM(b) op SNAP_AS_NUM(a));                                           \
			pop();                                                                                       \
		} else {                                                                                       \
			return binop_error(#op, b, a);                                                               \
		}                                                                                              \
	} while (false);

#define BIT_BINOP(op)                                                                              \
	Value& b = PEEK(1);                                                                              \
	Value& a = PEEK(2);                                                                              \
                                                                                                   \
	if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {                                                         \
		SNAP_SET_NUM(a, SNAP_CAST_INT(a) op SNAP_CAST_INT(b));                                         \
		pop();                                                                                         \
	} else {                                                                                         \
		return binop_error(#op, a, b);                                                                 \
	}

#ifdef SNAP_DEBUG_RUNTIME
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
				return binop_error("/", b, a);
			}
			break;
		}

		case Op::mod: {
			Value& a = PEEK(1);
			Value& b = PEEK(2);

			if (SNAP_IS_NUM(a) and SNAP_IS_NUM(b)) {
				SNAP_SET_NUM(b, fmod(SNAP_AS_NUM(b), SNAP_AS_NUM(a)));
			} else {
				return binop_error("%", b, a);
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
			push(SNAP_BOOL_VAL(a == b));
			break;
		}

		case Op::neq: {
			Value a = pop();
			Value b = pop();
			push(SNAP_BOOL_VAL(a != b));
			break;
		}

		case Op::negate: {
			Value& operand = PEEK(1);
			if (SNAP_IS_NUM(operand))
				SNAP_SET_NUM(operand, -SNAP_AS_NUM(operand));
			else
				UNOP_ERROR("-", operand);
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
			Closure* cl = static_cast<Closure*>(m_current_frame->func);
			*cl->get_upval(idx)->m_value = PEEK(1);
			break;
		}

		case Op::get_upval: {
			u8 idx = NEXT_BYTE();
			SNAP_ASSERT(m_current_frame->func->tag == OT::func, "enclosing frame a snap closure!");
			Closure* cl = static_cast<Closure*>(m_current_frame->func);
			push(*cl->get_upval(idx)->m_value);
			break;
		}

		case Op::set_global: {
			Value name = READ_VALUE();
			SNAP_ASSERT(SNAP_IS_STRING(name), "Variable name not a string.");
			set_global(SNAP_AS_STRING(name), PEEK(1));
			break;
		}

		case Op::get_global: {
			Value name = READ_VALUE();
			SNAP_ASSERT_OT(name, OT::string);
			push(get_global(SNAP_AS_STRING(name)));
			break;
		}

		case Op::close_upval: {
			close_upvalues_upto(sp - 1);
			pop();
			break;
		}

		case Op::concat: {
			Value& a = PEEK(2);
			Value b = pop();

			if (!(SNAP_IS_STRING(a) and SNAP_IS_STRING(b))) {
				return binop_error("..", a, b);
			} else {
				auto left = SNAP_AS_STRING(a);
				auto right = SNAP_AS_STRING(b);

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
			push(SNAP_OBJECT_VAL(&make<Table>()));
			break;
		}

		case Op::table_add_field: {
			Value value = pop();
			Value key = pop();

			Value vtable = PEEK(1);
			SNAP_AS_TABLE(vtable)->set(key, value);
			break;
		}

		// table[key] = value
		case Op::index_set: {
			Value value = pop();
			Value key = pop();
			if (SNAP_IS_NIL(key)) return ERROR("Table key cannot be nil.");

			Value& tvalue = PEEK(1);
			if (SNAP_IS_TABLE(tvalue)) {
				SNAP_AS_TABLE(tvalue)->set(key, value);
				sp[-1] = value; // assignment returns it's RHS.
			} else {
				return ERROR("Attempt to index a {} value.", SNAP_TYPE_CSTR(tvalue));
			}
			break;
		}

		// table.key = value
		case Op::table_set: {
			const Value& key = READ_VALUE();
			if (SNAP_IS_NIL(key)) return ERROR("Table key cannot be nil.");
			Value value = pop();
			Value& tvalue = PEEK(1);
			if (SNAP_IS_TABLE(tvalue)) {
				SNAP_AS_TABLE(tvalue)->set(key, value);
				sp[-1] = value; // assignment returns it's RHS
			} else {
				return INDEX_ERROR(tvalue);
			}
			break;
		}

		// table.key
		case Op::table_get: {
			// TOS = as_table(TOS)->get(READ_VAL())
			Value tvalue = PEEK(1);
			if (SNAP_IS_TABLE(tvalue)) {
				sp[-1] = SNAP_AS_TABLE(tvalue)->get(READ_VALUE());
			} else {
				return INDEX_ERROR(tvalue);
			}
			break;
		}

		// table.key
		case Op::table_get_no_pop: {
			// push((TOS)->get(READ_VAL()))
			Value tval = PEEK(1);
			if (SNAP_IS_TABLE(tval)) {
				push(SNAP_AS_TABLE(tval)->get(READ_VALUE()));
			} else {
				return INDEX_ERROR(tval);
			}
			break;
		}

		// table_or_array[key]
		case Op::index: {
			Value key = pop();
			Value tvalue = PEEK(1);
			if (SNAP_IS_TABLE(tvalue)) {
				sp[-1] = SNAP_AS_TABLE(tvalue)->get(key);
			} else {
				return INDEX_ERROR(tvalue);
			}
			break;
		}

		// table_or_array[key]
		case Op::index_no_pop: {
			Value& vtable = PEEK(2);
			const Value& key = PEEK(1);
			if (SNAP_IS_TABLE(vtable)) {
				if (SNAP_IS_NIL(key)) return ERROR("Table key cannot be nil.");
				push(SNAP_AS_TABLE(vtable)->get(key));
			} else {
				return INDEX_ERROR(vtable);
			}
			break;
		}

		case Op::pop_jmp_if_false: {
			ip += IS_VAL_FALSY(PEEK(1)) ? FETCH_SHORT() : 2;
			pop();
			break;
		}

		// tbl <- POP()
		// PUSH(tbl[READ_VALUE()])
		// PUSH(tbl)
		case Op::prep_method_call: {
			Value vtable = PEEK(1);
			Value vkey = READ_VALUE();
			SNAP_ASSERT(SNAP_IS_STRING(vkey), "method name not a string.");
			if (!SNAP_IS_TABLE(vtable)) return INDEX_ERROR(vtable);
			sp[-1] = SNAP_AS_TABLE(vtable)->get(vkey);
			push(vtable);
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
			m_current_block = &static_cast<Closure*>(m_current_frame->func)->m_proto->block();
			ip = m_current_frame->ip;

			break;
		}

		case Op::make_func: {
			Value vproto = READ_VALUE();
			SNAP_ASSERT_OT(vproto, OT::proto);
			u32 num_upvals = NEXT_BYTE();
			Closure* func = &make<Closure>(SNAP_AS_PROTO(vproto), num_upvals);

			push(SNAP_OBJECT_VAL(func));

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
			SNAP_ERROR("Impossible opcode.");
			return ExitCode::RuntimeError;
		}
		}
#ifdef SNAP_DEBUG_RUNTIME
		print_stack(m_stack, sp - m_stack);
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
		Value vresult = SNAP_OBJECT_VAL(res);
		interned_strings.set(vresult, SNAP_BOOL_VAL(true));
		return vresult;
	} else {
		delete[] buf;
		return SNAP_OBJECT_VAL(interned);
	}
}

Value VM::get_global(String* name) const {
	auto search = m_global_vars.find(name);
	if (search == m_global_vars.end()) return SNAP_NIL_VAL;
	return search->second;
}

Value VM::get_global(const char* name) {
	String& sname = string(name, strlen(name));
	return get_global(&sname);
}

void VM::set_global(String* name, Value value) {
	m_global_vars[name] = value;
}

void VM::set_global(const char* name, Value value) {
	String& sname = string(name, strlen(name));
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

	Prototype* proto = m_compiler->compile();
	// If the compilation failed, return false
	// and signal a compile time error.
	if (!compiler.ok()) return false;

	// There are no reachable references to `proto`
	// when we allocate `func`. Since allocating a func
	// can trigger a garbage collection cycle, we protect
	// the proto.
	gc_protect(proto);
	Closure* func = &make<Closure>(proto, 0);

	// Once the function has been made, proto can
	// be reached via `func->m_proto`, so we can
	// unprotect it.
	gc_unprotect(proto);

	push(SNAP_OBJECT_VAL(func));
	callfunc(func, 0);

#ifdef SNAP_DEBUG_DISASSEMBLY
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
	auto vglobal = SNAP_OBJECT_VAL(o);
	// setting a global variable may trigger a garbage collection
	// cycle (when allocating the [name] as snap::String). At that
	// point, vprint is only reachable on the C stack, so we protect
	// it by pushing it on to the VM stack.
	push(vglobal);
	set_global(name, vglobal);
	pop();
}

void VM::load_stdlib() {
	add_stdlib_object("print", &make<CClosure>(stdlib::print));
	add_stdlib_object("setmeta", &make<CClosure>(stdlib::setmeta));
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
	if (!SNAP_CHECK_TT(value, VT::Object))
		ERROR("Attempt to call a {} value.", SNAP_TYPE_CSTR(value));

	switch (SNAP_AS_OBJECT(value)->tag) {
	case OT::func: return callfunc(SNAP_AS_CLOSURE(value), argc);
	case OT::cfunc: return call_cclosure(SNAP_AS_CCLOSURE(value), argc);
	default: ERROR("Attempt to call a {} value.", SNAP_TYPE_CSTR(value)); return false;
	}

	return false;
}

void VM::push_callframe(Obj* callable, int argc) {
	SNAP_ASSERT(callable->tag == OT::cfunc or callable->tag == OT::func,
							"Non callable callframe pushed.");

	// Save the current instruction pointer
	// in the call frame so we can resume
	// execution when the function returns.
	m_current_frame->ip = ip;
	// prepare the next call frame
	m_current_frame = &m_frames[m_frame_count++];
	m_current_frame->func = callable;
	m_current_frame->base = sp - argc - 1;

	// start from the first opcode
	m_current_frame->ip = ip = 0;

	if (callable->tag == OT::func) {
		Closure* cl = static_cast<Closure*>(callable);
		m_current_block = &cl->m_proto->block();
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
	if (callable->tag == OT::func) {
		Closure* cl = static_cast<Closure*>(callable);
		m_current_block = &cl->m_proto->block();
	}
}

bool VM::callfunc(Closure* func, int argc) {
	int extra = argc - func->m_proto->param_count();

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

	push_callframe(func, argc);
	return true;
}

bool VM::call_cclosure(CClosure* cclosure, int argc) {
	push_callframe(cclosure, argc);
	CFunction c_func = cclosure->cfunc();
	Value ret = c_func(*this, argc);
	pop_callframe();

	popn(argc);
	sp[-1] = ret;

	return !m_has_error;
}

String& VM::string(const char* chars, size_t length) {
	size_t hash = hash_cstring(chars, length);

	// If an identical string has already been created, then
	// return a reference to the existing string instead.
	String* interned = interned_strings.find_string(chars, length, hash);
	if (interned != nullptr) return *interned;

	String* string = &make<String>(chars, length, hash);
	interned_strings.set(SNAP_OBJECT_VAL(string), SNAP_BOOL_VAL(true));

	return *string;
}

// 	-- Garbage collection --

size_t VM::collect_garbage() {
	m_gc.mark();
	m_gc.trace();
	return m_gc.sweep();
}

// -- Error reporting --

ExitCode VM::binop_error(const char* opstr, Value& a, Value& b) {
	return ERROR("Cannot use operator '{}' on operands of type '{}' and '{}'.", opstr,
							 SNAP_TYPE_CSTR(a), SNAP_TYPE_CSTR(b));
}

ExitCode VM::runtime_error(std::string const& message) {
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

		const Block& block = func.m_proto->block();
		SNAP_ASSERT(frame.ip >= 0 and frame.ip < block.lines.size(),
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

} // namespace snap