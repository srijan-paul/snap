#include "lib_util.hpp"
#include "../str_format.hpp"
#include <vm.hpp>


namespace vyse {

void bad_arg_error(vyse::VM& vm, const char* fname, int argn, const char* expected_type,
									 const char* received_type) {
	vm.runtime_error(kt::format_str("Bad argument #{} to '{}' expected {}, got {}.", argn, fname,
																	expected_type, received_type));
}

} // namespace vyse