#include "lib_util.hpp"
#include "value.hpp"
#include <std/vystrlib.hpp>
#include <vm.hpp>

namespace vyse::stdlib {

using namespace vyse::stdlib::util;

Value byte(VM& vm, int argc) {
	static constexpr const char* fname = "byte";

	if (argc < 1) {
		cfn_error(vm, fname, "too few arguments to (needs 1 string argument).");
		return VYSE_NIL;
	}

	const Value& strval = vm.get_arg(0);
	if (!check_arg_type(vm, 0, ObjType::string, fname)) return VYSE_NIL;

	const String& string = *VYSE_AS_STRING(strval);
	if (string.len() == 0) {
		cfn_error(vm, fname, "string must be at least one character long.");
		return VYSE_NIL;
	}

	const char c = string.at(0);
	return VYSE_NUM(c);
}

} // namespace vyse::stdlib