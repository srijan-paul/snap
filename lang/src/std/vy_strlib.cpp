#include "lib_util.hpp"
#include "value.hpp"
#include <std/vy_strlib.hpp>
#include <vm.hpp>

namespace vyse::stdlib {

Value byte(VM& vm, int argc) {
	if (argc < 1) {
		vm.runtime_error("too few arguments to function 'byte' (needs 1 string argument)");
		return VYSE_NIL;
	}

	const Value& strval = vm.get_arg(0);
	if (!VYSE_IS_STRING(strval)) {
		bad_arg_error(vm, "byte", 1, "string", VYSE_TYPE_CSTR(strval));
		return VYSE_NIL;
	}

	const String& string = *VYSE_AS_STRING(strval);
	if (string.len() == 0) {
		vm.runtime_error("string must be at least one character long in 'byte'.");
		return VYSE_NIL;
	}

	const char c = string.at(0);
	return VYSE_NUM(c);
}

} // namespace vyse::stdlib