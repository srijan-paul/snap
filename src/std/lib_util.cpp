#include "../str_format.hpp"
#include "value.hpp"
#include <std/lib_util.hpp>
#include <vm.hpp>

namespace vy::util {

void bad_arg_error(vy::VM& vm, const char* fname, int argn, const char* expected_type,
				   const char* received_type) {
	vm.runtime_error(kt::format_str("Bad argument #{} to '{}' expected {}, got {}.", argn, fname,
									expected_type, received_type));
}

void add_libfn(VM& vm, Table& proto, const char* name, NativeFn cfn) {
	vm.gc_off();
	String* sname = &vm.make_string(name);
	CClosure* fn = &vm.make<CClosure>(cfn);
	proto.set(VYSE_OBJECT(sname), VYSE_OBJECT(fn));
	vm.gc_on();
}

static bool check_arg_type(VM& vm, int argn, ValueType expected_type, const char* expected_type_str,
						   const char* fname) {
	const Value& v_arg = vm.get_arg(argn);
	if (VYSE_CHECK_TT(v_arg, expected_type)) return true;
	bad_arg_error(vm, fname, argn + 1, expected_type_str, value_type_name(v_arg));
	return false;
}

bool check_arg_type(VM& vm, int argn, ValueType expected_type, const char* fname) {
	const char* vtype_name = vtype_to_string(expected_type);
	return check_arg_type(vm, argn, expected_type, vtype_name, fname);
}

bool check_arg_type(VM& vm, int argn, ObjType expected_type, const char* fname) {
	const Value& v_arg = vm.get_arg(argn);
	if (VYSE_IS_OBJECT(v_arg) and VYSE_AS_OBJECT(v_arg)->tag == expected_type) return true;
	bad_arg_error(vm, fname, argn + 1, otype_to_string(expected_type), value_type_name(v_arg));
	return false;
}

bool check_argc(VM& vm, const char* fname, int argc, int min_args, int max_args) {
	assert(min_args >= 0);
	if (max_args == -1) max_args = min_args;

	if (min_args == max_args and argc != min_args) {
		vm.runtime_error(
			kt::format_str("In {}: Expected exactly {} args (got {}).", fname, min_args, argc));
		return false;
	}

	if (argc < min_args or argc > max_args) {
		vm.runtime_error(kt::format_str("In {}: Expected {}-{} args (got {}).", fname, min_args,
										max_args, argc));
		return false;
	}

	return true;
}

void cfn_error(VM& vm, const char* fname, std::string&& message) {
	vm.runtime_error(kt::format_str("In call to {}: {}", fname, message));
}

} // namespace vy::util
