#include "../lib_util.hpp"
#include <std/primitives/vy_number.hpp>
#include <vm.hpp>

namespace vyse::stdlib::primitives {

using namespace util;

Value to_str(VM& vm, int argc) {
	constexpr const char* fname = "to_string";

	if (argc != 1) {
		cfn_error(vm, "", "No argument provided. (expected 1 number)");
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number num = VYSE_AS_NUM(vm.get_arg(0));
	char* num_cstr = num_to_cstring(num);
	String* str = &vm.take_string(num_cstr, strlen(num_cstr));

	return VYSE_OBJECT(str);
}

void load_num_proto(VM& vm) {
	Table& num_proto = *vm.prototypes.number;
	add_libfn(vm, num_proto, "to_string", to_str);
}

} // namespace vyse::stdlib::primitives