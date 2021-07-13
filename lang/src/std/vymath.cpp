#include <cmath>
#include <limits>
#include <random>
#include <std/lib_util.hpp>
#include <type_traits>
#include <util/auxlib.hpp>
#include <vm.hpp>

using namespace vyse;
using namespace vyse::util;

namespace vyse::stdlib::math {

static constexpr number pi = 3.141592653589793238462643383279502884197;
static constexpr number e = 2.7182818284590452354;
static constexpr number vy_nan = std::numeric_limits<number>::quiet_NaN();
static constexpr number vy_max_num = std::numeric_limits<number>::max();
static constexpr number vy_min_num = std::numeric_limits<number>::min();
static constexpr number vy_inf = std::numeric_limits<number>::infinity();

Value sqrt(VM& vm, int argc) {
	Args args(vm, "math.sqrt", 1, argc);
	return VYSE_NUM(std::sqrt(args.next_number()));
}

template <typename T>
T vy_random(T lo = 0, T hi = 1) {
	static_assert(std::is_arithmetic_v<T>);
	std::random_device rd;
	if constexpr (std::is_floating_point_v<T>) {
		std::uniform_real_distribution<T> dist(lo, hi);
		return dist(rd);
	} else {
		std::uniform_int_distribution<T> dist(lo, hi);
		return dist(rd);
	}
}

Value random(VM& vm, int argc) {
	static constexpr const char* fname = "math.random";

	if (argc != 0 and argc != 2) {
		cfn_error(vm, fname, "Expected 0 or 2 arguments.");
		return VYSE_NIL;
	}

	if (argc == 0) return VYSE_NUM(vy_random<number>());

	assert(argc == 2);
	if (!check_arg_type(vm, 0, ValueType::Number, fname) or
		!check_arg_type(vm, 1, ValueType::Number, fname)) {
		return VYSE_NIL;
	}

	number low = VYSE_AS_NUM(vm.get_arg(0));
	number high = VYSE_AS_NUM(vm.get_arg(1));

	return VYSE_NUM(vy_random<number>(low, high));
}

Value randint(VM& vm, int argc) {
	Args args(vm, "math.randint", 2, argc);

	number low = args.next_number();
	number high = args.next_number();
	return VYSE_NUM(vy_random<int64_t>(low, high + 1));
}

Value sin(VM& vm, int argc) {
	Args args(vm, "math.sin", 1, argc);
	return VYSE_NUM(std::sin(args.next_number()));
}

Value cos(VM& vm, int argc) {
	Args args(vm, "math.cos", 1, argc);
	return VYSE_NUM(std::cos(args.next_number()));
}

Value tan(VM& vm, int argc) {
	Args args(vm, "math.tan", 1, argc);
	return VYSE_NUM(std::tan(args.next_number()));
}

Value asin(VM& vm, int argc) {
	Args args(vm, "math.asin", 1, argc);
	return VYSE_NUM(std::asin(args.next_number()));
}

Value acos(VM& vm, int argc) {
	Args args(vm, "math.acos", 1, argc);
	return VYSE_NUM(std::acos(args.next_number()));
}

Value atan(VM& vm, int argc) {
	Args args(vm, "math.atan", 1, argc);
	return VYSE_NUM(std::atan(args.next_number()));
}

Value max(VM& vm, int argc) {
	Args args(vm, "math.max", argc, argc);
	args.check(argc >= 1, "Expected 1 or more arguments");

	number max = args.next_number();
	while (args.has_next()) {
		max = std::max(max, args.next_number());
	}

	return VYSE_NUM(max);
}

Value min(VM& vm, int argc) {
	Args args(vm, "math.min", 1, argc); // We assume there is at-least 1 argument
	args.check(argc >= 1, "Expected 1 or more arguments");

	number min = args.next_number();
	while (args.has_next()) {
		min = std::min(min, args.next_number());
	}

	return VYSE_NUM(min);
}

Value isnan(VM& vm, int argc) {
	Args args(vm, "math.isnan", 1, argc);
	return VYSE_BOOL(std::isnan(args.next_number()));
}

Value isinf(VM& vm, int argc) {
	Args args(vm, "math.isinf", 1, argc);
	return VYSE_BOOL(std::isinf(args.next_number()));
}

Value log(VM& vm, int argc) {
	Args args(vm, "math.log", 1, argc); // need at least 1 arg

	const number x = args.next_number();
	if (argc == 2) {
		const number base = args.next_number();
		if (base == 10.0) return VYSE_NUM(std::log10(x));
		if (base == 2.0) return VYSE_NUM(std::log2(x));
		return VYSE_NUM(std::log(x) / std::log(base));
	}

	return VYSE_NUM(std::log(x));
}

Value log10(VM& vm, int argc) {
	Args args(vm, "math.log10", 1, argc);
	return VYSE_NUM(std::log10(args.next_number()));
}

Value exp(VM& vm, int argc) {
	Args args(vm, "math.exp", 1, argc);
	return VYSE_NUM(expf(args.next_number()));
}

Value todeg(VM& vm, int argc) {
	static constexpr number factor = 180.0 / pi;
	Args args(vm, "math.todeg", 1, argc);
	return VYSE_NUM(args.next_number() * factor);
}

Value torad(VM& vm, int argc) {
	static constexpr number factor = pi / 180.0;
	Args args(vm, "math.torad", 1, argc);
	return VYSE_NUM(args.next_number() * factor);
}

Value atan2(VM& vm, int argc) {
	Args args(vm, "math.atan2", 2, argc);
	return VYSE_NUM(std::atan2(args.next_number(), args.next_number()));
}

Value tan2(VM& vm, int argc) {
	Args args(vm, "math.tan2", 2, argc);
	return VYSE_NUM(std::tan(args.next_number()) * std::tan(args.next_number()));
}

s64 powmod(s64 x, s64 y, s64 mod) {
	s64 result = 1;
	while (y) {
		if (y & 1) {
			result = (result * x) % mod;
		}
		y >>= 1;
		x = (x * x) % mod;
	}
	return result;
}

Value pow(VM& vm, int argc) {
	Args args(vm, "math.pow", 2, argc);
	const number base = args.next_number();
	const number power = args.next_number();
	if (args.has_next()) {
		const number mod = args.next_number();
		args.check(is_integer(power) && is_integer(mod) && is_integer(base),
				   "Expected integer base, power and modulus");
		args.check(mod >= 0, "3rd argument (modulus) must be positive.");
		return VYSE_NUM(powmod(base, power, mod));
	}

	return VYSE_NUM(std::pow(base, power));
}

// fast combination algorithm
// https://en.wikipedia.org/wiki/Combination
int64_t vy_comb(int64_t n, int64_t k) {
	if (k > n) {
		return 0;
	}
	if (k > n / 2) {
		k = n - k;
	}
	int64_t result = 1;
	for (int64_t i = 1; i <= k; ++i) {
		result = (result * (n - i + 1)) / i;
	}
	return result;
}

Value comb(VM& vm, int argc) {
	Args args(vm, "math.comb", 2, argc);
	const number n = args.next_number();
	const number k = args.next_number();
	args.check(is_integer(n) && is_integer(k), "Expected integer arguments");
	return VYSE_NUM(vy_comb(static_cast<int64_t>(n), static_cast<int64_t>(k)));
}

static constexpr std::pair<const char*, NativeFn> funcs[] = {
	{"sqrt", sqrt}, {"random", random}, {"randint", randint}, {"sin", sin},		{"cos", cos},
	{"tan", tan},	{"asin", asin},		{"acos", acos},		  {"atan", atan},	{"math", atan},
	{"atan", atan}, {"max", max},		{"min", min},		  {"isnan", isnan}, {"isinf", isinf},
	{"log", log},	{"log10", log10},	{"exp", exp},		  {"todeg", todeg}, {"torad", torad},
	{"tan2", tan2}, {"atan2", atan2},	{"pow", pow},		  {"comb", comb},
};

VYSE_API void load_math(VM* vm, Table* module) {
	assert(vm != nullptr and module != nullptr);
	NativeModule math(vm, module);

	math.add_cclosures(funcs, array_size(funcs));

	math.add_field("pi", VYSE_NUM(pi));
	math.add_field("nan", VYSE_NUM(vy_nan));
	math.add_field("inf", VYSE_NUM(vy_inf));
	math.add_field("e", VYSE_NUM(e));
	math.add_field("max_value", VYSE_NUM(vy_max_num));
	math.add_field("min_value", VYSE_NUM(vy_min_num));
}

} // namespace vyse::stdlib::math