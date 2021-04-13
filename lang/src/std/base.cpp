#include "../str_format.hpp"
#include <std/base.hpp>
#include <vm.hpp>

static void bad_arg_error(snap::VM& vm, const char* fname, int argn, const char* expected_type,
													const char* received_type) {
	vm.runtime_error(kt::format_str("Bad argument #{} to '{}' expected {}, got {}.", argn, fname,
																	expected_type, received_type));
}

/// TODO: benchmark and optimize this.
snap::Value snap::stdlib::print(VM& vm, int argc) {
	std::string res = "";
	for (int i = 0; i < argc; ++i) {
		res += value_to_string(vm.get_arg(i)) + "\t";
	}

	res += "\n";
	String& snap_str = vm.string(res.c_str(), res.size());
	vm.print(vm, &snap_str);

	return SNAP_NIL_VAL;
}

snap::Value snap::stdlib::setmeta(VM& vm, int argc) {
	static const char* func_name = "setmeta";

	SNAP_ASSERT(argc == 2, "Incorrect number of arguments for function 'setmeta'.");
	Value& vtable = vm.get_arg(0);
	Value& vmeta = vm.get_arg(1);

	if (!SNAP_IS_TABLE(vtable)) {
		bad_arg_error(vm, func_name, 1, "table", SNAP_TYPE_CSTR(vmeta));
		return SNAP_NIL_VAL;
	}

	if (!SNAP_IS_TABLE(vmeta)) {
		bad_arg_error(vm, func_name, 2, "table", SNAP_TYPE_CSTR(vmeta));
		return SNAP_NIL_VAL;
	}

	SNAP_AS_TABLE(vtable)->m_meta_table = SNAP_AS_TABLE(vmeta);
	return vtable;
}