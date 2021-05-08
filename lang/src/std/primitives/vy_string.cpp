#include "../../str_format.hpp"
#include "../lib_util.hpp"
#include <vm.hpp>

namespace vyse::stdlib::primitives {

using namespace util;

#define FMT(...) kt::format_str(__VA_ARGS__)

Value substr(VM& vm, int argc) {
	static constexpr const char* fname = "String::substr";

	if (argc < 2 or argc > 3) {
		vm.runtime_error(FMT("Expected 2-3 arguments for call to String:substr (found {})", argc));
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ObjType::string, fname)) return VYSE_NIL;
	if (!check_arg_type(vm, 1, ValueType::Number, fname)) return VYSE_NIL;

	Value const& vstring = vm.get_arg(0);
	Value const& vfrom = vm.get_arg(1);

	const String* str = VYSE_AS_STRING(vstring);

	number from_arg = VYSE_AS_NUM(vfrom);
	size_t from = from_arg;
	if (from_arg < 0 or from_arg != u64(from_arg)) {
		cfn_error(vm, fname, "argument #2 ('from_index') must be a non-negative whole number");
		return VYSE_NIL;
	}

	number len_arg = str->len() - from;
	size_t len = len_arg;
	if (argc == 3) {
		if (!check_arg_type(vm, 2, ValueType::Number, fname)) return VYSE_NIL;
		len_arg = VYSE_AS_NUM(vm.get_arg(2));

		if (len_arg < 0 or len_arg != u64(len_arg)) {
			cfn_error(vm, fname, "argument #3 ('substr_length') must be a non-negative whole number");
			return VYSE_NIL;
		}

		len = len_arg;
	}

	if (from + len > str->len()) {
		cfn_error(vm, fname, "substring index out of bounds");
		return VYSE_NIL;
	}

	char* buf = new char[len + 1];
	for (size_t i = 0; i < len; ++i) {
		buf[i] = str->at(from + i);
	}
	buf[len] = '\0';

	String* sub = &vm.take_string(buf, len);
	return VYSE_OBJECT(sub);
}

void init_string_proto(VM& vm, Table& str_proto) {
	add_libfn(vm, str_proto, "substr", substr);
}

}