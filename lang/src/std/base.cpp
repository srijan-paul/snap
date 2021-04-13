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

snap::Value snap::stdlib::setproto(VM& vm, int argc) {
	static const char* func_name = "setmeta";

	if (argc != 2) {
		vm.runtime_error("'setmeta' builtin expects exactly 2 arguments.");
		return SNAP_NIL_VAL;
	}

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

	// Check for cyclic meta tables.
	Table* table = SNAP_AS_TABLE(vtable);
	const Table* metatable = SNAP_AS_TABLE(vmeta);
	while (metatable != nullptr) {
		if (metatable == table) {
			vm.runtime_error("Cyclic metatables are not allowed.");
			return SNAP_NIL_VAL;
		}
		metatable = metatable->m_proto_table;
	}

	table->m_proto_table = SNAP_AS_TABLE(vmeta);
	return vtable;
}