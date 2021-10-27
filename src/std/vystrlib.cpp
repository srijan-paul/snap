#include <std/lib_util.hpp>
#include <util/args.hpp>
#include "value.hpp"
#include <std/vystrlib.hpp>
#include <vm.hpp>

namespace vy::stdlib {

using namespace vy::util;

Value byte(VM& vm, int argc) {
	Args args(vm, "byte", 1, argc);
	const String& string = args.next<String>();
	args.check(string.len() > 0, "String must be at least one character long.");
	const char c = string.at(0);
	return VYSE_NUM(c);
}

} // namespace vyse::stdlib
