#include "util/args.hpp"
#include <stdlib/vy_number.hpp>
#include <util/lib_util.hpp>
#include <vm.hpp>

namespace vy::stdlib::primitives {

using namespace util;

Value to_str(VM& vm, int argc) {
	Args args(vm, "Number.to_string", 1, argc);
	const number num = args.next_number();
	char* const num_cstr = num_to_cstring(num);
	String* const str = &vm.take_string(num_cstr, strlen(num_cstr));
	return VYSE_OBJECT(str);
}

void load_num_proto(VM& vm) {
	Table& num_proto = *vm.prototypes.number;
	add_libfn(vm, num_proto, "to_string", to_str);
}

} // namespace vy::stdlib::primitives
