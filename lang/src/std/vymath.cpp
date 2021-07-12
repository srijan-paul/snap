#include "util/native_module.hpp"
#include "value.hpp"
#include <cmath>
#include <common.hpp>
#include <random>
#include <std/lib_util.hpp>
#include <type_traits>
#include <util/args.hpp>
#include <vm.hpp>

using namespace vyse;
using namespace vyse::util;

#define CHECK_ARGC(...)                                                                            \
	if (!check_argc(vm, fname, argc, __VA_ARGS__)) {                                               \
		return VYSE_NIL;                                                                           \
	}

Value math_sqrt(VM& vm, int argc) {
	Args args(vm, "math.sqrt", 1, argc);
	return VYSE_NUM(sqrt(args.next_number()));
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
	Args args(vm, "math.randint", 2, argc);

	number low = args.next_number();
	number high = args.next_number();
	return VYSE_NUM(floor(random<number>(low, high + 1)));
}

Value math_sin(VM& vm, int argc) {
	Args args(vm, "math.sin", 1, argc);
	return VYSE_NUM(sin(args.next_number()));
}

Value math_cos(VM& vm, int argc) {
	Args args(vm, "math.cos", 1, argc);
	return VYSE_NUM(cos(args.next_number()));
}

Value math_tan(VM& vm, int argc) {
	Args args(vm, "math.tan", 1, argc);
	return VYSE_NUM(tan(args.next_number()));
}

Value math_asin(VM& vm, int argc) {
	Args args(vm, "math.asin", 1, argc);
	return VYSE_NUM(asin(args.next_number()));
}

Value math_acos(VM& vm, int argc) {
	Args args(vm, "math.acos", 1, argc);
	return VYSE_NUM(acos(args.next_number()));
}

Value math_atan(VM& vm, int argc) {
	Args args(vm, "math.atan", 1, argc);
	return VYSE_NUM(atan(args.next_number()));
}

VYSE_API void load_math(VM* vm, Table* module) {
	assert(vm != nullptr and module != nullptr);
	NativeModule math(vm, module);

	math.add_cfunc("sqrt", math_sqrt);
	math.add_cfunc("random", math_random);
	math.add_cfunc("randint", math_randint);
	math.add_cfunc("sin", math_sin);
	math.add_cfunc("cos", math_cos);
	math.add_cfunc("tan", math_tan);
	math.add_cfunc("asin", math_asin);
	math.add_cfunc("acos", math_acos);
	math.add_cfunc("atan", math_atan);
	math.add_field("pi", VYSE_NUM(3.14159265358979323846));
}
