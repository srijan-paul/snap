#include "../../str_format.hpp"
#include "../lib_util.hpp"
#include <std/primitives/vy_string.hpp>
#include <vm.hpp>

namespace vyse::stdlib::primitives {

using namespace util;

#define FMT(...) kt::format_str(__VA_ARGS__)

Value substr(VM& vm, int argc) {
	static constexpr const char* fname = "String.substr";

	if (argc < 2 or argc > 3) {
		vm.runtime_error(FMT("Expected 2-3 arguments for call to String:substr (found {})", argc));
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ObjType::string, fname)) return VYSE_NIL;
	if (!check_arg_type(vm, 1, ValueType::Number, fname)) return VYSE_NIL;

	const Value& vstring = vm.get_arg(0);
	const Value& vfrom = vm.get_arg(1);

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

Value code_at(VM& vm, int argc) {
	static constexpr const char* fname = "String.byte";

	if (argc < 2) {
		cfn_error(vm, fname, "too few arguments. expected at least 2 (string, index).");
		return VYSE_NIL;
	}

	/// check the first argument.
	const Value& strval = vm.get_arg(0);
	if (!check_arg_type(vm, 0, ObjType::string, fname)) return VYSE_NIL;
	const String& string = *VYSE_AS_STRING(strval);
	if (string.len() == 0) {
		cfn_error(vm, fname, "argument [string] must be at least one character long.");
		return VYSE_NIL;
	}

	/// check the second argument.
	const Value& v_index = vm.get_arg(1);
	if (!check_arg_type(vm, 1, ValueType::Number, fname)) return VYSE_NIL;

	number index = VYSE_AS_NUM(v_index);
	if (index < 0 or index >= string.len()) {
		cfn_error(vm, fname, "string index out of bounds.");
		return VYSE_NIL;
	}

	if (index != u64(index)) {
		cfn_error(vm, fname, "index must be a non-negative whole number.");
		return VYSE_NIL;
	}

	const char c = string.at(index);
	return VYSE_NUM(c);
}

void load_string_proto(VM& vm) {
	Table& str_proto = *vm.primitive_protos.string;
	add_libfn(vm, str_proto, "substr", substr);
	add_libfn(vm, str_proto, "code_at", code_at);
}

} // namespace vyse::stdlib::primitives