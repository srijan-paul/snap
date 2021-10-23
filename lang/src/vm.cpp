#include "loadlib.hpp"
#include "std/primitives/vy_list_proto.hpp"
#include "std/vystrlib.hpp"
#include "str_format.hpp"
#include "util.hpp"
#include "value.hpp"
#include <cmath>
#include <cstddef>
#include <std/base.hpp>
#include <std/primitives/vy_number.hpp>
#include <std/primitives/vy_string.hpp>
#include <util/args.hpp>
#include <vm.hpp>
#include <vy_list.hpp>

#if defined(VYSE_DEBUG_RUNTIME) || defined(VYSE_DEBUG_DISASSEMBLY)
#include <cstdio>
#include <debug.hpp>
#endif

#define ERROR(...) runtime_error(kt::format_str(__VA_ARGS__))
#define INDEX_ERROR(v) ERROR("Attempt to index a '{}' value.", VYSE_TYPE_CSTR(v))
#define CURRENT_LINE() (m_current_block->lines[ip - 1])

#define CHECK_TYPE(v, typ, ...)                                                                    \
	if (!VYSE_CHECK_TT(v, typ)) {                                                                  \
		return ERROR(__VA_ARGS__);                                                                 \
	}

#define CHECK(cond, ...)                                                                           \
	if (!(cond)) {                                                                                 \
		return ERROR(__VA_ARGS__);                                                                 \
	}

#define FETCH() (m_current_block->code[ip++])
#define NEXT_BYTE() (static_cast<u8>(m_current_block->code[ip++]))
#define FETCH_SHORT()                                                                              \
	(ip += 2, (u16)((static_cast<u8>(m_current_block->code[ip - 2]) << 8) |                        \
					static_cast<u8>(m_current_block->code[ip - 1])))
#define READ_VALUE() (m_current_block->constant_pool[NEXT_BYTE()])
#define GET_VAR(index) (m_current_frame->base[index])
#define SET_VAR(index, value) (m_current_frame->base[index] = value)

// PEEK(1) fetches the topmost value in the stack.
#define PEEK(depth) m_stack.top[-(depth)]
#define POP() (m_stack.pop())
#define DISCARD() (--m_stack.top)
#define POPN(n) (m_stack.top -= n)
#define PUSH(value) m_stack.push(value)

namespace vyse {

using Op = Opcode;
using VT = ValueType;
using OT = ObjType;

#define IS_VAL_FALSY(v) ((VYSE_IS_BOOL(v) and !(VYSE_AS_BOOL(v))) or VYSE_IS_NIL(v))
#define IS_VAL_TRUTHY(v) (!IS_VAL_FALSY(v))

#define UNOP_ERROR(op, v) ERROR("Cannot use operator '{}' on type '{}'.", op, VYSE_TYPE_CSTR(v))

#define CMP_OP(op, proto_method)                                                                   \
	do {                                                                                           \
		Value& b = PEEK(1);                                                                        \
		Value& a = PEEK(2);                                                                        \
                                                                                                   \
		if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {                                                   \
			m_stack.top[-2] = (VYSE_BOOL(VYSE_AS_NUM(a) op VYSE_AS_NUM(b)));                       \
			DISCARD();                                                                             \
		} else if (!call_binary_overload(#op, proto_method)) {                                     \
			return ExitCode::RuntimeError;                                                         \
		}                                                                                          \
	} while (false);

#define BINOP(op, proto_method_name)                                                               \
	do {                                                                                           \
		Value& r = PEEK(1);                                                                        \
		Value& l = PEEK(2);                                                                        \
		/* Do not pop any values yet, we may still need them for GC */                             \
		if (VYSE_IS_NUM(l) and VYSE_IS_NUM(r)) {                                                   \
			VYSE_SET_NUM(l, VYSE_AS_NUM(l) op VYSE_AS_NUM(r));                                     \
			DISCARD();                                                                             \
		} else if (!call_binary_overload(#op, proto_method_name)) {                                \
			return ExitCode::RuntimeError;                                                         \
		}                                                                                          \
	} while (false);

#define BIT_BINOP(op, proto_method_name)                                                           \
	Value& b = PEEK(1);                                                                            \
	Value& a = PEEK(2);                                                                            \
                                                                                                   \
	if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {                                                       \
		VYSE_SET_NUM(a, VYSE_CAST_INT(a) op VYSE_CAST_INT(b));                                     \
		DISCARD();                                                                                 \
	} else if (!call_binary_overload(#op, proto_method_name)) {                                    \
		return ExitCode::RuntimeError;                                                             \
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
		case Op::add: BINOP(+, "__add"); break;
		case Op::sub: BINOP(-, "__sub"); break;
		case Op::mult: BINOP(*, "__mult"); break;

		case Op::gt: CMP_OP(>, "__gt"); break;
		case Op::lt: CMP_OP(<, "__lt"); break;
		case Op::gte: CMP_OP(>=, "__gte"); break;
		case Op::lte: CMP_OP(<=, "__lte"); break;

		case Op::div: {
			Value& a = PEEK(1);
			Value& b = PEEK(2);

			if (VYSE_IS_NUM(a) and VYSE_IS_NUM(b)) {
				if (VYSE_AS_NUM(b) == 0) {
					return runtime_error("Attempt to divide by 0.\n");
				}
				VYSE_SET_NUM(b, VYSE_AS_NUM(b) / VYSE_AS_NUM(a));
				DISCARD();
			} else if (!call_binary_overload("/", "__div")) {
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
			} else if (!call_binary_overload("/", "__exp")) {
				return binop_error("**", base, power);
			}
			break;
		}

		case Op::mod: {
			Value& l = PEEK(2);
			Value& r = PEEK(1);

			if (VYSE_IS_NUM(l) and VYSE_IS_NUM(r)) {
				VYSE_SET_NUM(l, fmod(VYSE_AS_NUM(l), VYSE_AS_NUM(r)));
				DISCARD();
			} else if (!call_binary_overload("%", "__mod")) {
				return binop_error("%", l, r);
			}
			break;
		}

		case Op::lshift: {
			BIT_BINOP(<<, "__bsl");
			break;
		}

		case Op::rshift: {
			BIT_BINOP(>>, "__bsr");
			break;
		}

		case Op::band: {
			BIT_BINOP(&, "__band");
			break;
		}

		case Op::bxor: {
			BIT_BINOP(^, "__bxor");
			break;
		}

		case Op::bor: {
			BIT_BINOP(|, "__bor");
			break;
		}

		/// TODO: overload with __eq
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
			if (VYSE_IS_NUM(operand)) {
				VYSE_SET_NUM(operand, -VYSE_AS_NUM(operand));
			} else if (!call_unary_overload("__negate")) {
				return UNOP_ERROR("-", operand);
			}
			break;
		}

		case Op::lnot: {
			Value a = POP();
			PUSH(VYSE_BOOL(IS_VAL_FALSY(a)));
			break;
		}

		case Op::len: {
			Value v = POP();
			if (VYSE_IS_LIST(v)) {
				PUSH(VYSE_NUM(VYSE_AS_LIST(v)->length()));
			} else if (VYSE_IS_TABLE(v)) {
				/// TODO: overload with __len
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
				return ERROR("Cannot use operator '~' on value of type '{}'",
							 VYSE_TYPE_CSTR(PEEK(1)));
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

		// In a for loop, the variables are to be set up in the stack as such:
		// [counter, limit, step, i].
		// 'i' is the user exposed counter. The user is free to modify this
		// variable, however the 'for_loop' instruction at the end of a loop's body will change it
		// back to `counter + limit`.  This instruction performs the following operations:
		// 1. counter = counter - 1;
		// 2. i = counter;
		// 3. jump to to corresponding for_loop opcode;
		case Op::for_prep: {
			Value& counter = PEEK(3);
			CHECK_TYPE(counter, VT::Number, "'for' variable not a number.");
			CHECK_TYPE(PEEK(2), VT::Number, "'for' limit not a number.");
			const Value& step = PEEK(1);
			CHECK_TYPE(step, VT::Number, "'for' step not a number.");
			VYSE_SET_NUM(counter, VYSE_AS_NUM(counter) - VYSE_AS_NUM(step));
			PUSH(counter); // load the user exposed loop counter (i).
			// jump to the corresponding for_loop instruction.
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
			VYSE_ASSERT(VYSE_IS_STRING(name), "global name not a string.");
			set_global(VYSE_AS_STRING(name), POP());
			break;
		}

		case Op::get_global: {
			Value name = READ_VALUE();
			VYSE_ASSERT(VYSE_IS_STRING(name), "global name not a string.");
			Value value = get_global(VYSE_AS_STRING(name));
			if (VYSE_IS_UNDEFINED(value)) {
				return ERROR("Undefined variable '{}'.", VYSE_AS_STRING(name)->c_str());
			}
			PUSH(value);
			break;
		}

		case Op::close_upval: {
			close_upvalues_upto(m_stack.top - 1);
			DISCARD();
			break;
		}

		case Op::concat: {
			Value& a = PEEK(2);
			Value b = POP();

			if (!(VYSE_IS_STRING(a) and VYSE_IS_STRING(b))) {
				return binop_error("..", a, b);
			} else {
				auto l = VYSE_AS_STRING(a);
				auto r = VYSE_AS_STRING(b);

				// The second string has been popped off the stack and might not be reachable by
				// the GC. The allocation of the concatenated string might trigger a GC cycle.
				GCLock _ = gc_lock(r);
				a = concatenate(l, r);
			}
			break;
		}

		case Op::new_list: {
			PUSH(VYSE_OBJECT(&make<List>()));
			break;
		}

		case Op::list_append: {
			Value& vlist = PEEK(2);
			if (VYSE_IS_LIST(vlist)) {
				VYSE_AS_LIST(vlist)->append(POP());
			} else {
				return ERROR("Attempt to append to a {} value. (Can only append to lists)",
							 value_type_name(vlist));
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

		// table_or_list[key] = value
		case Op::subscript_set: {
			Value rhs = POP();
			Value key = POP();
			Value& lhs = PEEK(1);

			bool ok = subscript_set(lhs, key, rhs);
			// assignment returns it's RHS.
			m_stack.top[-1] = ok ? rhs : VYSE_NIL;
			break;
		}

		/// table.key = value
		/// TODO: overload with `__set`
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
		/// TODO: overload with `__get`
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
		/// TODO: overload with `__indx`
		case Op::subscript_get: {
			Value key = POP();
			Value& tvalue = PEEK(1);
			if (!get_subscript_of_value(tvalue, key, tvalue)) {
				return ExitCode::RuntimeError;
			}
			break;
		}

		case Op::index_no_pop: {
			const Value& value = PEEK(2);
			const Value& key = PEEK(1);
			Value result;
			if (!get_subscript_of_value(value, key, result)) {
				return ExitCode::RuntimeError;
			}
			PUSH(result);
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
		/// TODO: take care of overloaded `__indx`
		case Op::prep_method_call: {
			Value vtable = PEEK(1);
			Value vkey = READ_VALUE();
			VYSE_ASSERT(VYSE_IS_STRING(vkey), "method name not a string.");

			if (VYSE_IS_NIL(vtable)) return INDEX_ERROR(vtable);
			if (VYSE_IS_TABLE(vtable)) {
				m_stack.top[-1] = VYSE_AS_TABLE(vtable)->get(vkey);
			} else {
				m_stack.top[-1] = index_proto(vtable, vkey);
			}
			PUSH(vtable);

			break;
		}

		case Op::call_func: {
			u8 argc = NEXT_BYTE();
			Value value = PEEK(argc + 1);
			if (!op_call(value, argc)) return ExitCode::RuntimeError;
			break;
		}

		case Op::return_val: {
			Value result = POP();
			close_upvalues_upto(m_current_frame->base);
			m_stack.top = m_current_frame->base + 1;

			DISCARD();
			PUSH(result);

			// No more code to run, the script has executed successfully.
			m_frame_count--;
			if (m_frame_count == 0) {
				return_value = result;
				return ExitCode::Success;
			}

			m_current_frame = m_current_frame->prev;
			VYSE_ASSERT(m_current_frame != nullptr, "Invalid call stack state.");

			// If the call site of this Vyse function was in C++ then we return control to the C++
			// function.
			if (m_current_frame->func->tag == OT::c_closure) {
				return ExitCode::Success;
			}

			VYSE_ASSERT(m_current_frame->func->tag == OT::closure,
						"Invalid callable object at callframe base.");
			m_current_block = &static_cast<Closure*>(m_current_frame->func)->m_codeblock->block();
			ip = m_current_frame->ip;
			break;
		}

		case Op::make_func: {
			Value vcode = READ_VALUE();
			VYSE_ASSERT(VYSE_IS_CODEBLOCK(vcode), "make_func arg not a codeblock.");
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
	const size_t length = left->len() + right->len();

	char* const buf = new char[length + 1];
	buf[length] = '\0';

	std::memcpy(buf, left->c_str(), left->len());
	std::memcpy(buf + left->len(), right->c_str(), right->len());

	const size_t hash = hash_cstring(buf, length);
	String* const interned = interned_strings.find_string(buf, length, hash);

	if (interned == nullptr) {
		String* const res = &make_string_no_intern(buf, length, hash);
		Value vresult = VYSE_OBJECT(res);
		interned_strings.set(vresult, VYSE_BOOL(true));
		return vresult;
	} else {
		delete[] buf;
		return VYSE_OBJECT(interned);
	}
}

Value VM::get_global(String* name) const {
	const auto search = m_global_vars.find(name);
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
	// When allocating space for the [name] string, a GC cycle may be triggered, so
	// we have to protect the object from getting collected while that happens.
	if (VYSE_IS_OBJECT(value)) m_stack.push(value);
	String& sname = make_string(name, strlen(name));
	if (VYSE_IS_OBJECT(value)) m_stack.pop();

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
#undef PUSH
#undef DISCARD
#undef POP

ExitCode VM::interpret() {
	const bool ok = init();
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

	Closure* const script = compile(*m_source);
	if (script == nullptr) return false;
	invoke_script(script);

#ifdef VYSE_DEBUG_DISASSEMBLY
	disassemble_block(script->name()->c_str(), *m_current_block);
	printf("\n");
#endif

	return true;
}

Closure* VM::compile(const std::string& src) {
	Compiler compiler{this, &src};
	m_compiler = &compiler;

	CodeBlock* const code = m_compiler->compile();

	if (!compiler.ok()) {
		m_compiler = nullptr;
		return nullptr;
	}

	// There are no reachable references to [code] when we allocate `script`. Since allocating a
	// function can trigger a garbage collection cycle, we protect the code block.
	GCLock const lock = gc_lock(code);
	Closure* const closure = &make<Closure>(code, 0);

	m_compiler = nullptr;
	return closure;
}

void VM::invoke_script(Closure* script) {
	// clear the stack in case there is some leftover junk from previous invocations.
	m_stack.clear();
	// make sure there is enough room in the stack for this function call. +1 for the script itself.
	ensure_slots(script->m_codeblock->stack_size() + 1);
	m_stack.push(VYSE_OBJECT(script));
	base_frame->base = m_stack.top - 1;
	base_frame->ip = 0;
	ip = 0;
	base_frame->func = script;
	m_frame_count = 1;
	m_current_frame = base_frame;
	m_current_block = &script->m_codeblock->block();
}

ExitCode VM::runcode(const std::string& code) {
	m_source = &code;
	return interpret();
}

void VM::add_stdlib_object(const char* name, Obj* o) {
	Value const vglobal = VYSE_OBJECT(o);
	set_global(name, vglobal);
}

void VM::load_stdlib() {
	add_stdlib_object("print", &make<CClosure>(stdlib::print));
	add_stdlib_object("setproto", &make<CClosure>(stdlib::setproto));
	add_stdlib_object("getproto", &make<CClosure>(stdlib::getproto));
	add_stdlib_object("byte", &make<CClosure>(stdlib::byte));
	add_stdlib_object("assert", &make<CClosure>(stdlib::assert_));
	add_stdlib_object("input", &make<CClosure>(stdlib::input));
	add_stdlib_object("import", &make<CClosure>(stdlib::import));

	// Initialize the default package loader functions used by the `import` builtin.
	dynloader.init_loaders(*this);

	load_primitives();
}

void VM::load_primitives() {
	/// load string prototype.
	prototypes.string = &make<Table>();
	set_global("String", VYSE_OBJECT(prototypes.string));
	stdlib::primitives::load_string_proto(*this);

	/// load number prototype.
	prototypes.number = &make<Table>();
	set_global("Number", VYSE_OBJECT(prototypes.number));
	stdlib::primitives::load_num_proto(*this);

	/// load boolean prototype.
	prototypes.boolean = &make<Table>();
	set_global("Bool", VYSE_OBJECT(prototypes.boolean));

	/// load list prototype.
	prototypes.list = &make<Table>();
	set_global("List", VYSE_OBJECT(prototypes.list));
	stdlib::primitives::load_list_proto(*this);
}

using OT = ObjType;

Upvalue* VM::capture_upvalue(Value* slot) {
	// start at the head of the linked list
	Upvalue* current = m_open_upvals;
	Upvalue* prev = nullptr;

	// keep going until we reach a slot whose depth is lower than what we've been looking for, or
	// until we reach the end of the list.
	while (current != nullptr and current->m_value < slot) {
		prev = current;
		current = current->next_upval;
	}

	// We've found an upvalue that was already capturing a value at this stack slot, so we reuse the
	// existing upvalue
	if (current != nullptr and current->m_value == slot) return current;

	// We've reached a node in the list where the previous node is above the slot we wanted to
	// capture, but the current node is deeper. Meaning `slot` points to a new value that hasn't
	// been captured before. So we add it between `prev` and `current`.
	Upvalue* const upval = &make<Upvalue>(slot);
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
		Upvalue* const current = m_open_upvals;
		// these two lines are the last rites of an upvalue, closing it.
		current->closed = *current->m_value;
		current->m_value = &current->closed;
		m_open_upvals = current->next_upval;
	}
}

bool VM::call(int argc) {
	VYSE_ASSERT(argc < (m_stack.top - m_stack.values),
				"Invalid stack state or incorrect arg count.");

	const Value& value = m_stack.peek(argc + 1);
	const bool ok = op_call(value, argc);

	// If the called object was a CClosure then execution has already finished and no need to call
	// run() from here.
	if (VYSE_IS_CCLOSURE(value)) return ok;
	const ExitCode ec = run();
	return ec == ExitCode::Success;
}

bool VM::op_call(Value value, u8 argc) {
	if (VYSE_IS_NIL(value)) {
		ERROR("Attempt to call a nil value.");
		return false;
	}

	if (m_frame_count >= MaxCallStack) {
		ERROR("Stack overflow.");
		return false;
	}

	if (VYSE_IS_OBJECT(value)) {
		switch (VYSE_AS_OBJECT(value)->tag) {
		case OT::closure: return call_closure(VYSE_AS_CLOSURE(value), argc);
		case OT::c_closure: return call_cclosure(VYSE_AS_CCLOSURE(value), argc);
		default: break; // fall
		}
	}

	// not a function, so we get it's `__call` field and attempt to call it
	const bool ok = call_func_overload(value, argc);
	if (!ok) ERROR("Attempt to call a {} value.", VYSE_TYPE_CSTR(value));
	return ok;
}

void VM::push_callframe(Obj* callee, int argc) {
	VYSE_ASSERT(callee->tag == OT::c_closure or callee->tag == OT::closure,
				"Non callable callframe pushed.");
	VYSE_ASSERT(argc >= 0, "Negative argument count.");

	// Save the current instruction pointer in the current call frame so we can resume execution
	// when the next function returns.
	m_current_frame->ip = ip;

	// prepare the next call frame
	if (m_current_frame->next) {
		m_current_frame = m_current_frame->next;
	} else {
		CallFrame* const next_cf = new CallFrame;
		m_current_frame->next = next_cf;
		next_cf->prev = m_current_frame;
		m_current_frame = next_cf;
	}

	++m_frame_count;

	m_current_frame->func = callee;
	m_current_frame->base = m_stack.top - argc - 1;

	// Start new function from the first opcode
	m_current_frame->ip = ip = 0;

	if (callee->tag == OT::closure) {
		Closure* const cl_obj = static_cast<Closure*>(callee);
		m_current_block = &cl_obj->m_codeblock->block();
	}
}

void VM::pop_callframe() noexcept {
	VYSE_ASSERT(m_frame_count > 1, "Attempt to pop base callframe.");
	--m_frame_count;

	// If we are in the top level script, then there is no older call frame.
	if (m_frame_count == 0) return;

	m_current_frame = m_current_frame->prev;
	VYSE_ASSERT(m_current_frame != nullptr, "Invalid Call stack state.");

	// restore the instruction pointer to continue from where we left off.
	ip = m_current_frame->ip;

	Obj* const callable = m_current_frame->func;
	if (callable->tag == OT::closure) {
		Closure* cl = static_cast<Closure*>(callable);
		m_current_block = &cl->m_codeblock->block();
	}
}

bool VM::call_closure(Closure* func, int num_args) {
	const int num_params = func->m_codeblock->param_count();

	// make sure there is enough room in the stack for this function call.
	ensure_slots(func->m_codeblock->stack_size());

	// extra arguments are ignored and missing arguments are padded with 'nil'.
	if (num_args < num_params) {
		// some parameters are missing
		while (num_args != num_params) {
			m_stack.push(VYSE_NIL);
			num_args++;
		}
	} else if (func->m_codeblock->is_vararg()) {
		num_args = prep_vararg_call(num_params, num_args);
	} else {
		while (num_args != num_params) {
			m_stack.pop();
			num_args--;
		}
	}

	assert(num_args == num_params);
	push_callframe(func, num_args);
	return true;
}

int VM::prep_vararg_call(int num_params, int num_args) {
	VYSE_ASSERT(num_args >= num_params, "bad call to VM::prep_vararg_call");
	List& vararg_list = make<List>();
	int num_varargs = num_args - num_params + 1;
	for (Value* arg = m_stack.top - num_varargs; arg < m_stack.top; ++arg) {
		vararg_list.append(*arg);
	}
	m_stack.popn(num_varargs);
	m_stack.push(VYSE_OBJECT(&vararg_list));
	return num_params;
}

bool VM::call_cclosure(CClosure* cclosure, int argc) {
	push_callframe(cclosure, argc);
	NativeFn c_func = cclosure->cfunc();

	Value ret;
	try {
		ret = c_func(*this, argc);
	} catch (const util::CArityException& ex) {
		// incorrect number of arguments.
		ERROR("In call to '{}': Expected {} arguments. Got {}.", ex.cfunc_name, ex.num_params,
			  argc);
		ret = VYSE_NIL;
	} catch (const util::CTypeException& ex) {
		ERROR("Bad argument #{} to '{}' expected {}, got {}.", ex.argn, ex.func_name,
			  ex.expected_type_name, ex.received_type_name);
		ret = VYSE_NIL;
	} catch (const util::CMiscException& ex) {
		ERROR("In call to '{}': {}", ex.fname, ex.message);
		ret = VYSE_NIL;
	}

	pop_callframe();

	m_stack.popn(argc);
	m_stack.top[-1] = ret;

	return !m_has_error;
}

bool VM::call_binary_overload(const char* op_str, const char* method_name) {
	/// TODO: get rid of the temporary string object here
	const Value overload_name = VYSE_OBJECT(&make_string(method_name));
	const Value r = m_stack.pop();
	const Value l = m_stack.pop();

	// Look for an overloaded method on each of the operands.
	// First check the left operand, then the right.
	Value overload_fn = index_proto(l, overload_name);
	if (VYSE_IS_NIL(overload_fn)) overload_fn = index_proto(r, overload_name);
	if (VYSE_IS_NIL(overload_fn)) {
		binop_error(op_str, l, r);
		return false;
	}

	ensure_slots(3);
	m_stack.push(overload_fn);
	m_stack.push(l);
	m_stack.push(r);

	// Call the overload function with 2 arguments, each corresponding to an operand
	return op_call(overload_fn, 2);
}

bool VM::call_unary_overload(const char* method_name) {
	const Value mname = VYSE_OBJECT(&make_string(method_name));
	const Value value = m_stack.pop();

	const Value method = index_proto(value, mname);
	if (VYSE_IS_NIL(method)) return false;

	ensure_slots(2);
	m_stack.push(method);
	m_stack.push(value);

	return op_call(method, 1);
}

bool VM::call_func_overload(Value& object, int argc) {
	static constexpr const char* overload_name = "__call";
	static String& method_string = make_string(overload_name);
	static const Value field_name = VYSE_OBJECT(&method_string);

	Value func;
	bool ok = get_field_of_value(object, field_name, func);
	assert(ok);

	// To perform the overloaded call, we need to adjust the stack
	// from [object, args...] to [func, object, args...].
	ensure_slots(1);
	m_stack.push(VYSE_NIL);
	for (int index = 1; index <= argc + 1; ++index) {
		m_stack.top[-index] = m_stack.top[-index - 1];
	}

	m_stack.top[-argc - 2] = func;
	return op_call(func, argc + 1);
}

bool VM::get_field_of_value(Value const& value, Value const& key, Value& inout) {
	VYSE_ASSERT(VYSE_IS_STRING(key), "field name not a string");

	if (VYSE_IS_TABLE(value)) {
		Table* const object = VYSE_AS_TABLE(value);
		inout = object->get(key);
		return true;
	}

	Table* const proto = get_proto(value);
	if (proto == nullptr) {
		assert(VYSE_IS_NIL(value));
		ERROR("Attempt to index a nil value.");
		return false;
	}

	inout = proto->get(key);
	return true;
}

bool VM::get_subscript_of_value(const Value& value, const Value& index, Value& result) {
	if (VYSE_IS_OBJECT(value)) {
		Obj* const object = VYSE_AS_OBJECT(value);
		switch (object->tag) {
		case OT::table: {
			Table* const table = static_cast<Table*>(object);
			result = table->get(index);
			return true;
		}

		case OT::list: {
			const List* list = static_cast<List*>(object);
			if (not VYSE_IS_NUM(index)) {
				ERROR("List index not a number.");
				return false;
			}

			const number idx = VYSE_AS_NUM(index);
			if (idx < 0 or idx >= list->length()) {
				ERROR("List index out of bounds. (index: {}, length: {})", idx, list->length());
				return false;
			}
			result = list->at(idx);
			return true;
		}

		case OT::string: {
			const String& string = *static_cast<String*>(object);
			if (not VYSE_IS_NUM(index)) {
				ERROR("String index not a number");
				return false;
			}

			const number idx = VYSE_AS_NUM(index);
			if (idx < 0 or idx >= string.m_length) {
				ERROR("String index out of bounds. (index: {}, length: {})", idx, string.m_length);
				return false;
			}

			result = VYSE_OBJECT(char_at(&string, idx));
			return true;
		}

		default: break; // fallthrough to default
		}
	}

	// Find prototype of primitive value and index it with [index]
	Table* const proto = get_proto(value);
	assert(proto->tag == OT::table);
	if (proto == nullptr) {
		ERROR("Attempt to index a {} value.", VYSE_TYPE_CSTR(value));
		return false;
	}
	result = proto->get(index);
	return true;
}

bool VM::subscript_set(const Value& lhs, const Value& key, const Value& rhs) {
	if (VYSE_IS_LIST(lhs)) {
		List& list = *VYSE_AS_LIST(lhs);
		return list_index_set(list, key, rhs);
	}

	if (VYSE_IS_TABLE(lhs)) {
		if (VYSE_IS_NIL(key)) {
			ERROR("Table key cannot be nil.");
			return false;
		}
		VYSE_AS_TABLE(lhs)->set(key, rhs);
		return true;
	}

	ERROR("Attempt to index a {} value.", VYSE_TYPE_CSTR(lhs));
	return false;
}

bool VM::list_index_set(List& list, const Value& key, const Value& value) {
	if (!VYSE_CHECK_TT(key, VT::Number)) {
		ERROR("List index not a number.");
		return false;
	}

	number index = VYSE_AS_NUM(key);
	if (index < 0 or index >= list.length()) {
		ERROR("List index out of bounds (index: {}, length: {}).", index, list.length());
		return false;
	}

	list[index] = value;
	return true;
}

// ---------------------------
// STRING MANIPULATION HELPERS
// ---------------------------

String* VM::char_at(const String* string, uint index) {
	char* charbuf = new char[2]{string->at(index), '\0'};
	return &take_string(charbuf, 1);
}

String& VM::take_string(char* buf, size_t len) {
	size_t hash = hash_cstring(buf, len);

	// Look for an existing interened copy of the string.
	String* interned = interned_strings.find_string(buf, len, hash);
	if (interned != nullptr) {
		// We now 'own' the string, so we are free to get rid of this buffer if we don't need it.
		delete[] buf;
		return *interned;
	}

	String& string = make_string_no_intern(buf, len, hash);
	interned_strings.set(VYSE_OBJECT(&string), VYSE_BOOL(true));
	return string;
}

String& VM::make_string(const char* chars, size_t length) {
	const size_t hash = hash_cstring(chars, length);

	// If an identical string has already been created, then return a reference to the existing
	// string instead.
	String* const interned = interned_strings.find_string(chars, length, hash);
	if (interned != nullptr) return *interned;

	String* const string = &make_string_no_intern(chars, length, hash);
	interned_strings.set(VYSE_OBJECT(string), VYSE_BOOL(true));

	return *string;
}

/*
 * Growing the VM stack is done by `realloc`ing the old stack buffer to a new
 * location in memory. However, when we do so, we must be careful enough to update
 * all pointers that pointed to the old stack. Let's use call frames as an example.
 * Every call frame has a 'base' pointer, that points to the first slot in the stack
 * which belongs to the frame. When the stack is moved to a new location, this
 * 'base' pointer must also be updated.
 *
 * The key idea is that since the contents and state of the stack are preserved on
 * growth, so the distance between the old stack's base and the callframe's base is
 * equal to the distance between the new stack's base and the callframe's base.
 *
 * dH = call_frame.base - old_stack.base
 * call_frame.base = new_stack.base + dH
 *
 *                                                                    +--------+
 *                                                                    |        |
 *                                                                    |--------|
 * +-------+                                                          |        |
 * |       |      (BEFORE)                         (AFTER)            |--------|
 * |-------| <- call_frame.base -+                                    |        |
 * |       |                     |            +- call_frame.base ->   |--------|
 * |-------|                     |  dH1 = dH2 |                       |        |
 * |       |                     |            |                       |--------|
 * +-------+ <- old_stack.base  -+            |                       |        |
 *                                            +- new_stack.base  ->   +--------+
 * OLD STACK                                                          NEW STACK
 *
 * Similarly, we also update the pointers in the upvalue chain of the VM.
 *
 */
void VM::ensure_slots(uint num_requested_slots) {
	const std::ptrdiff_t num_used_slots = m_stack.top - m_stack.values;
	const uint num_free_slots = m_stack.size - num_used_slots;

	// Requested number of slots is already available.
	if (num_free_slots > num_requested_slots) return;

	Value* const old_stack_base = m_stack.values;
	const uint new_stack_size = pow2ceil(m_stack.size + (num_requested_slots - num_free_slots) + 1);
	m_stack.size = new_stack_size;
	m_stack.values = static_cast<Value*>(realloc(m_stack.values, m_stack.size * sizeof(Value)));

	// Now that the stack has moved in memory, the CallFrames and the Upvalue chain still contain
	// dangling pointers to the old stack, so we update those to the same relative distance from the
	// new stack's base address.
	for (CallFrame* cf = m_current_frame; cf; cf = cf->prev) {
		cf->base = m_stack.values + (cf->base - old_stack_base);
	}

	for (Upvalue* upval = m_open_upvals; upval != nullptr; upval = upval->next_upval) {
		upval->m_value = (upval->m_value - old_stack_base) + m_stack.values;
	}

	m_stack.top = m_stack.values + num_used_slots;
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
	return ERROR("Bad types for operator '{}': '{}' and '{}'.", opstr, VYSE_TYPE_CSTR(a),
				 VYSE_TYPE_CSTR(b));
}

ExitCode VM::runtime_error(const std::string& message) {
	// avoid cascading errors if one error has already been reported
	if (m_has_error) return ExitCode::RuntimeError;

	m_has_error = true;

	std::string error_str =
		(m_current_frame->is_cclosure())
			? kt::format_str("[internal] {}\nstack trace:\n", message)
			: kt::format_str("[line {}]: {}\nstack trace:\n", CURRENT_LINE(), message);

	size_t trace_depth = 0;
	for (CallFrame* frame = m_current_frame; frame; frame = frame->prev) {
		++trace_depth;
		if (trace_depth >= MaxStackTraceDepth) {
			continue;
		}
		/// TODO: Handle CFunction strack traces more elegantly
		if (frame->is_cclosure()) continue;

		const Closure& func = *static_cast<Closure*>(frame->func);

		const Block& block = func.m_codeblock->block();
		VYSE_ASSERT(frame->ip < block.lines.size(),
					"IP not in range for std::vector<u32> block.lines.");

		int line = block.lines[frame->ip];
		if (frame == base_frame) {
			error_str += kt::format_str("\t[line {}] in {}", line, func.name_cstr());
		} else {
			error_str += kt::format_str("\t[line {}] in function {}.\n", line, func.name_cstr());
		}
	}

	if (trace_depth >= MaxStackTraceDepth) {
		size_t diff = trace_depth - MaxStackTraceDepth;
		error_str += "\t.\n\t.\n\t.\n\t" + std::to_string(diff) + " not shown.\n";
		Closure* scriptfn = static_cast<Closure*>(base_frame->func);
		int line = scriptfn->m_codeblock->block().lines[base_frame->ip];
		error_str += kt::format_str("\t[line {}] in function {}.\n", line, scriptfn->name_cstr());
	}

	on_error(*this, error_str);
	return ExitCode::RuntimeError;
}

// The default behavior on an error is to simply print it to the stderr.
void default_error_fn([[maybe_unused]] const VM& vm, const std::string& err_msg) {
	fprintf(stderr, "%s\n", err_msg.c_str());
}

char* default_readline(const VM&) {
	size_t buf_size = 8;
	char* buf = (char*)malloc(sizeof(char) * buf_size);

	char c;
	size_t nchars = 0;
	while ((c = getc(stdin)) and c != EOF and c != '\n' and c) {
		if (nchars >= buf_size) {
			buf_size *= 2;
			buf = (char*)realloc(buf, sizeof(char) * buf_size);
		}
		buf[nchars] = c;
		++nchars;
	}
	buf = (char*)realloc(buf, sizeof(char) * (nchars + 1));
	buf[nchars] = '\0';
	return buf;
}

std::string default_find_module(VM&, const char*) {
	return "return { x: 1 }";
}

/// TODO: The user might need some objects even after the VM has been destructed. Add support for
/// this.
VM::~VM() {
	if (m_gc.m_objects == nullptr) return;
	for (Obj* object = m_gc.m_objects; object != nullptr;) {
		Obj* const next = object->next;
		delete object;
		object = next;
	}

	for (CallFrame* cf = base_frame; cf != nullptr;) {
		CallFrame* const next = cf->next;
		delete cf;
		cf = next;
	}
}

} // namespace vyse
