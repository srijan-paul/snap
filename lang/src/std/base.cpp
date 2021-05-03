#include "../str_format.hpp"
#include <std/base.hpp>
#include <vm.hpp>

static void bad_arg_error(vyse::VM& vm, const char* fname, int argn, const char* expected_type,
													const char* received_type) {
	vm.runtime_error(kt::format_str("Bad argument #{} to '{}' expected {}, got {}.", argn, fname,
																	expected_type, received_type));
}

/// TODO: benchmark and optimize this.
vyse::Value vyse::stdlib::print(VM& vm, int argc) {
	std::string res = "";
	for (int i = 0; i < argc; ++i) {
		res += value_to_string(vm.get_arg(i)) + "\t";
	}

	res += "\n";
	String& vyse_str = vm.make_string(res.c_str(), res.size());
	vm.print(vm, &vyse_str);

	return VYSE_NIL_VAL;
}

vyse::Value vyse::stdlib::setproto(VM& vm, int argc) {
	static const char* func_name = "setproto";

	if (argc != 2) {
		vm.runtime_error("'setproto' builtin expects exactly 2 arguments.");
		return VYSE_NIL_VAL;
	}

	Value& vtable = vm.get_arg(0);
	Value& vproto = vm.get_arg(1);

	if (!VYSE_IS_TABLE(vtable)) {
		bad_arg_error(vm, func_name, 1, "table", VYSE_TYPE_CSTR(vtable));
		return VYSE_NIL_VAL;
	}

	if (!VYSE_IS_TABLE(vproto)) {
		bad_arg_error(vm, func_name, 2, "table", VYSE_TYPE_CSTR(vproto));
		return VYSE_NIL_VAL;
	}

	// Check for cyclic meta tables.
	Table* table = VYSE_AS_TABLE(vtable);
	const Table* prototype = VYSE_AS_TABLE(vproto);
	while (prototype != nullptr) {
		if (prototype == table) {
			vm.runtime_error("cyclic prototype chains are not allowed.");
			return VYSE_NIL_VAL;
		}
		prototype = prototype->m_proto_table;
	}

	table->m_proto_table = VYSE_AS_TABLE(vproto);
	return vtable;
}