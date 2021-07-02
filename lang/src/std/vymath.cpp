#include <cmath>
#include <common.hpp>
#include <std/lib_util.hpp>
#include <vm.hpp>

using namespace vyse;
using namespace stdlib::util;

Value vy_sqrt(VM& vm, int argc) {
	static constexpr const char* fname = "math.sqrt";

	if (argc != 1) {
		cfn_error(vm, fname, "Expected a number argument for math.sqrt");
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number arg = VYSE_AS_NUM(vm.get_arg(0));
	return VYSE_NUM(sqrt(arg));
}

Table& load_math(VM& vm) {
	Table& module = vm.make<Table>();
	add_libfn(vm, module, "sqrt", vy_sqrt);
	return module;
}