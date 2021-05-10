#include "../../str_format.hpp"
#include "../lib_util.hpp"
#include "value.hpp"
#include <std/primitives/vy_list_proto.hpp>
#include <vm.hpp>
#include <vy_list.hpp>

namespace vyse::stdlib::primitives {

using namespace util;

Value foreach (VM& vm, int argc) {
	static constexpr const char* fname = "foreach";

	if (argc != 2) {
		cfn_error(vm, fname, "Expected 2 arguments for List.foreach. (list, function)");
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ObjType::list, fname)) return VYSE_NIL;
	List& list = *VYSE_AS_LIST(vm.get_arg(0));

	Value& vfunc = vm.get_arg(1);
	if (!(VYSE_IS_CLOSURE(vfunc) or VYSE_IS_CCLOSURE(vfunc))) {
		cfn_error(vm, fname,
							kt::format_str("Bad arg #2. Expected function got {}.", value_type_name(vfunc)));
		return VYSE_NIL;
	}

	uint list_len = list.length();

	for (uint i = 0; i < list_len; ++i) {
		vm.m_stack.push(vfunc);
		vm.m_stack.push(list[i]);
		vm.m_stack.push(VYSE_NUM(i));
		bool ok = vm.call(2);
		if (!ok) return VYSE_NIL;
		// the result of the function call is now
		// on the stack, so we pop it off.
		vm.m_stack.pop();
	}

	return VYSE_NIL;
}

Value make(VM& vm, int argc) {
	constexpr const char* fname = "make";
	if (argc == 0) {
		return VYSE_OBJECT(&vm.make<List>());
	}

	if (!check_arg_type(vm, 0, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number n = VYSE_AS_NUM(vm.get_arg(0));
	if (n < 0) {
		cfn_error(vm, fname, "List size cannot be negative.");
		return VYSE_NIL;
	}

	return VYSE_OBJECT(&vm.make<List>(n));
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

void load_list_proto(VM& vm) {
	Table& list_proto = *vm.prototypes.list;
	add_libfn(vm, list_proto, "foreach", foreach);
	add_libfn(vm, list_proto, "make", make);
	add_libfn(vm, list_proto, "fill", fill);
}

} // namespace vyse::stdlib::primitives