#include "value.hpp"
#include <cmath>
#include <common.hpp>
#include <random>
#include <std/lib_util.hpp>
#include <type_traits>
#include <vm.hpp>

using namespace vyse;
using namespace stdlib::util;

#define CHECK_ARGC(...)                                                                            \
	if (!check_argc(vm, fname, argc, __VA_ARGS__)) {                                               \
		return VYSE_NIL;                                                                           \
	}

Value math_sqrt(VM& vm, int argc) {
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

template <typename T>
T random(T lo = 0, T hi = 1) {
	static_assert(std::is_arithmetic_v<T>);
	std::random_device rd;
	std::uniform_real_distribution<T> dist(lo, hi);
	return dist(rd);
}

Value math_random(VM& vm, int argc) {
	static constexpr const char* fname = "math.random";

	if (argc != 0 and argc != 2) {
		cfn_error(vm, fname, "Expected 0 or 2 arguments.");
		return VYSE_NIL;
	}

	if (argc == 0) return VYSE_NUM(random<number>());
	assert(argc == 2);
	if (!check_arg_type(vm, 0, ValueType::Number, fname) or
		!check_arg_type(vm, 1, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number low = VYSE_AS_NUM(vm.get_arg(0));
	number high = VYSE_AS_NUM(vm.get_arg(1));

	return VYSE_NUM(random<number>(low, high));
}

Value math_randint(VM& vm, int argc) {
	constexpr const char* fname = "math.randint";
	CHECK_ARGC(2);
	if (!check_arg_type(vm, 0, ValueType::Number, fname) or
		!check_arg_type(vm, 1, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number low = VYSE_AS_NUM(vm.get_arg(0));
	number high = VYSE_AS_NUM(vm.get_arg(1));
	return VYSE_NUM(floor(random<number>(low, high + 1)));
}

Value math_sin(VM& vm, int argc) {
	constexpr const char* fname = "math.sin";
	CHECK_ARGC(1);
	if (!check_arg_type(vm, 0, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number argv = VYSE_AS_NUM(vm.get_arg(0));
	return VYSE_NUM(sin(argv));
}

VYSE_API void load_math(VM* vm, Table* module) {
	assert(vm != nullptr and module != nullptr);
	add_libfn(*vm, *module, "sqrt", math_sqrt);
	add_libfn(*vm, *module, "random", math_random);
	add_libfn(*vm, *module, "randint", math_randint);
	add_libfn(*vm, *module, "sin", math_sin);
	
	// String& str_pi = vm->make_string("pi");
	// GCLock  lock   = vm->gc_lock(&str_pi);
	// module->set(str_pi, VYSE_NUM(3.14159265358979323846));

	// lib_add_field(vm, vm->make_string("pi"), VYSE_NUM(M_PI));
}

