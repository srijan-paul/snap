#include "../str_format.hpp"
#include "lib_util.hpp"
#include "value.hpp"
#include <std/base.hpp>
#include <vm.hpp>


using namespace vyse::stdlib::util;

/// TODO: benchmark and optimize this.
vyse::Value vyse::stdlib::print(VM& vm, int argc) {
	std::string res = "";
	for (int i = 0; i < argc; ++i) {
		res += value_to_string(vm.get_arg(i)) + "\t";
	}

	res += "\n";
	String& vyse_str = vm.make_string(res.c_str(), res.size());
	vm.print(vm, &vyse_str);

	return VYSE_NIL;
}

vyse::Value vyse::stdlib::setproto(VM& vm, int argc) {
	static const char* func_name = "setproto";

	if (argc != 2) {
		vm.runtime_error("'setproto' builtin expects exactly 2 arguments.");
		return VYSE_NIL;
	}

	Value& vtable = vm.get_arg(0);
	Value& vproto = vm.get_arg(1);

	if (!VYSE_IS_TABLE(vtable)) {
		bad_arg_error(vm, func_name, 1, "table", VYSE_TYPE_CSTR(vtable));
		return VYSE_NIL;
	}

	if (!VYSE_IS_TABLE(vproto)) {
		bad_arg_error(vm, func_name, 2, "table", VYSE_TYPE_CSTR(vproto));
		return VYSE_NIL;
	}

	// Check for cyclic prototypes.
	Table* table = VYSE_AS_TABLE(vtable);
	const Table* prototype = VYSE_AS_TABLE(vproto);
	while (prototype != nullptr) {
		if (prototype == table) {
			vm.runtime_error("cyclic prototype chains are not allowed.");
			return VYSE_NIL;
		}
		prototype = prototype->m_proto_table;
	}

	table->m_proto_table = VYSE_AS_TABLE(vproto);
	return vtable;
}

vyse::Value vyse::stdlib::getproto(VM &vm, int argc) {
	static constexpr const char* fname = "assert";

	if (argc != 1) {
		cfn_error(vm, fname, "Expected at least 1 argument of type table.");
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ObjType::table, fname)) {
		return VYSE_NIL;
	}

	const Table* table = VYSE_AS_TABLE(vm.get_arg(0));
	if (table->m_proto_table == nullptr) return VYSE_NIL;
	return VYSE_OBJECT(table->m_proto_table);
}

vyse::Value vyse::stdlib::assert_(VM& vm, int argc) {
	static constexpr const char* fname = "assert";

	if (argc < 1 or argc > 2) {
		cfn_error(vm, fname, "Incorrect argument count. Expected 1-2 arguments.");
		return VYSE_NIL;
	}

	const Value& cond = vm.get_arg(0);
	if (is_val_truthy(cond)) return cond;

	const char* message = "assertion failed!";
	if (argc == 2) {
		if (!check_arg_type(vm, 1, ObjType::string, fname)) return VYSE_NIL;
		message = VYSE_AS_STRING(vm.get_arg(1))->c_str();
	}

	vm.runtime_error(message);
	return VYSE_NIL;
}