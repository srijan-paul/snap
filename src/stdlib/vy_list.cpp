#include "../str_format.hpp"
#include "util/args.hpp"
#include "value.hpp"
#include <util/lib_util.hpp>
#include <stdlib/vy_list.hpp>
#include <vm.hpp>
#include <vy_list.hpp>

#define CHECK_ARG_TYPE(n, type)                                                                    \
	if (!check_arg_type(vm, n, type, fname)) return VYSE_NIL;

namespace vy::stdlib::primitives {

using namespace util;

Value foreach (VM& vm, int argc) {
	Args args(vm, "List.foreach", 2, argc);
	List& list = args.next<List>();
	Value vfunc = args.next_arg();

	uint list_len = list.length();
	for (uint i = 0; i < list_len; ++i) {
		vm.m_stack.push(vfunc);
		vm.m_stack.push(list[i]);
		vm.m_stack.push(VYSE_NUM(i));
		bool ok = vm.call(2);
		if (!ok) return VYSE_NIL;
		// the result of the function call is now on the stack, so we pop it off.
		vm.m_stack.pop();
	}

	return VYSE_NIL;
}

Value make(VM& vm, int argc) {
	Args args(vm, "List.make", 1, argc);
	std::size_t size = args.next_number();
	args.check(size > 0, "list size must be positive.");
	return VYSE_OBJECT(&vm.make<List>(size));
}

Value fill(VM& vm, int argc) {
	constexpr const char* fname = "fill";
	if (argc != 2) {
		cfn_error(vm, fname, "expected 2 arguments, list and value.");
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ObjType::list, fname)) return VYSE_NIL;
	List& list = *VYSE_AS_LIST(vm.get_arg(0));
	Value value = vm.get_arg(1);

	size_t list_len = list.length();
	for (uint i = 0; i < list_len; ++i) {
		list[i] = value;
	}

	return VYSE_NIL;
}

Value slice(VM& vm, int argc) {
	constexpr const char* fname = "List.slice";
	if (argc != 3) {
		cfn_error(vm, fname, "Expected 3 arguments (list, from, to)");
		return VYSE_NIL;
	}
	CHECK_ARG_TYPE(0, ObjType::list);
	CHECK_ARG_TYPE(1, ValueType::Number);
	CHECK_ARG_TYPE(2, ValueType::Number);

	const List& list = *VYSE_AS_LIST(vm.get_arg(0));
	number from = VYSE_AS_NUM(vm.get_arg(1));
	number to = VYSE_AS_NUM(vm.get_arg(2));

	if (!list.in_range(from)) {
		cfn_error(vm, fname, "Bad argument #2 (from). List index out of range.");
		return VYSE_NIL;
	}

	if (!list.in_range(to)) {
		cfn_error(vm, fname, "Bad argument #3 (to). List index out of range.");
		return VYSE_NIL;
	}

	List& slice = vm.make<List>();
	size_t start = from, limit = to;

	for (size_t i = start; i <= limit; ++i) {
		slice.append(list[i]);
	}

	return VYSE_OBJECT(&slice);
}

Value map(VM& vm, int argc) {
	constexpr const char* fname = "List.map";
	if (argc != 2) {
		cfn_error(vm, fname, "Expected at least 2 arguments (list, map_fn");
		return VYSE_NIL;
	}

	CHECK_ARG_TYPE(0, ObjType::list);

	Value& vfunc = vm.get_arg(1);
	if (!(VYSE_IS_CLOSURE(vfunc) or VYSE_IS_CCLOSURE(vfunc))) {
		cfn_error(vm, fname,
				  kt::format_str("Bad arg #2. Expected function, got {}.", value_type_name(vfunc)));
		return VYSE_NIL;
	}

	const List& list = *VYSE_AS_LIST(vm.get_arg(0));
	List& ret = vm.make<List>();

	/// The list we just created isn't reachable by
	/// the garbage collector from any root set, so
	/// we protect it.
	vm.gc_protect(&ret);
	for (uint i = 0; i < list.length(); ++i) {
		vm.m_stack.push(vfunc);
		vm.m_stack.push(list[i]);
		vm.m_stack.push(VYSE_NUM(i));
		bool ok = vm.call(2);
		if (!ok) {
			vm.gc_unprotect(&ret);
			return VYSE_NIL;
		}
		ret.append(vm.m_stack.pop());
	}

	/// once we return it, it's going to get
	/// pushed onto the stack and become reachable.
	vm.gc_unprotect(&ret);
	return VYSE_OBJECT(&ret);
}

Value reduce(VM& vm, int argc) {
	constexpr const char* fname = "reduce";
	if (argc != 3 and argc != 2) {
		cfn_error(vm, fname, "Expected 2 or 3 arguments (list, reduce_fn, [init]).");
		return VYSE_NIL;
	}

	CHECK_ARG_TYPE(0, ObjType::list);

	Value& vfunc = vm.get_arg(1);
	if (!(VYSE_IS_CLOSURE(vfunc) or VYSE_IS_CCLOSURE(vfunc))) {
		cfn_error(vm, fname,
				  kt::format_str("Bad arg #2. Expected function, got {}.", value_type_name(vfunc)));
		return VYSE_NIL;
	}

	const List& list = *VYSE_AS_LIST(vm.get_arg(0));

	// If no initial value is provided then consider the first
	// element of the array to be the initial value, and start
	// using the reducer function from the first value onwards.
	Value init;
	uint start_index;

	if (argc == 3) {
		init = vm.get_arg(2);
		start_index = 0;
	} else {
		init = list[0];
		start_index = 1;
	}

	for (uint i = start_index; i < list.length(); ++i) {
		vm.m_stack.push(vfunc);
		vm.m_stack.push(init);
		vm.m_stack.push(list[i]);
		vm.m_stack.push(VYSE_NUM(i));
		if (!vm.call(3)) return VYSE_NIL;
		init = vm.m_stack.pop();
	}

	return init;
}

Value filter(VM& vm, int argc) {
	constexpr const char* fname = "List.filter";
	if (argc != 2) {
		cfn_error(vm, fname, "Expected 2 arguments (list, filter_fn).");
		return VYSE_NIL;
	}

	CHECK_ARG_TYPE(0, ObjType::list);
	const List& list = *VYSE_AS_LIST(vm.get_arg(0));

	Value& vfunc = vm.get_arg(1);
	if (!(VYSE_IS_CLOSURE(vfunc) or VYSE_IS_CCLOSURE(vfunc))) {
		cfn_error(vm, fname,
				  kt::format_str("Bad arg #2. Expected function, got {}.", value_type_name(vfunc)));
		return VYSE_NIL;
	}

	List& ret = vm.make<List>();
	vm.gc_protect(&ret);

	for (uint i = 0; i < list.length(); ++i) {
		vm.m_stack.push(vfunc);
		vm.m_stack.push(list[i]);
		vm.m_stack.push(VYSE_NUM(i));
		if (!vm.call(2)) {
			vm.gc_unprotect(&ret);
			return VYSE_NIL;
		}
		Value res = vm.m_stack.pop();
		if (is_val_truthy(res)) {
			ret.append(list[i]);
		}
	}

	vm.gc_unprotect(&ret);
	return VYSE_OBJECT(&ret);
}

Value pop(VM& vm, int argc) {
	constexpr const char* fname = "pop";
	if (argc != 1) {
		cfn_error(vm, fname, kt::format_str("Expected exactly one argument to List.pop"));
		return VYSE_NIL;
	}

	CHECK_ARG_TYPE(0, ObjType::list);

	List& list = *VYSE_AS_LIST(vm.get_arg(0));
	if (list.length() == 0) {
		cfn_error(vm, fname, "Attempt to pop from an empty list");
		return VYSE_NIL;
	}

	return list.pop();
}

void load_list_proto(VM& vm) {
	Table& list_proto = *vm.prototypes.list;
	add_libfn(vm, list_proto, "foreach", foreach);
	add_libfn(vm, list_proto, "make", make);
	add_libfn(vm, list_proto, "fill", fill);
	add_libfn(vm, list_proto, "slice", slice);
	add_libfn(vm, list_proto, "map", map);
	add_libfn(vm, list_proto, "reduce", reduce);
	add_libfn(vm, list_proto, "filter", filter);
	add_libfn(vm, list_proto, "pop", pop);
}

} // namespace vy::stdlib::primitives
