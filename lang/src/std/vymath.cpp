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

template <typename T>
T random(T lo = 0, T hi = 1) {
	static_assert(std::is_arithmetic_v<T>);
	std::random_device rd;
	std::uniform_real_distribution<T> dist(lo, hi);
	return dist(rd);
}

Value vy_random(VM& vm, int argc) {
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

Value vy_randint(VM& vm, int argc) {
	constexpr const char* fname = "math.randint";
	CHECK_ARGC(2);
	if (!check_arg_type(vm, 0, ValueType::Number, fname) or
		!check_arg_type(vm, 0, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number low = VYSE_AS_NUM(vm.get_arg(0));
	number high = VYSE_AS_NUM(vm.get_arg(1));
	return VYSE_NUM(floor(random<number>(low, high + 1)));
}

VYSE_API void load_math(VM* vm, Table* module) {
	assert(vm != nullptr and module != nullptr);
	add_libfn(*vm, *module, "sqrt", vy_sqrt);
	add_libfn(*vm, *module, "random", vy_random);
	add_libfn(*vm, *module, "randint", vy_randint);
}