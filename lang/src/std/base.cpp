#include "../str_format.hpp"
#include "value.hpp"
#include "vy_list.hpp"
#include <loadlib.hpp>
#include <std/base.hpp>
#include <std/lib_util.hpp>
#include <vm.hpp>

using namespace vyse::util;
using namespace vyse;

/// TODO: benchmark and optimize this.
Value vyse::stdlib::print(VM& vm, int argc) {
	std::string res = "";
	for (int i = 0; i < argc; ++i) {
		res += value_to_string(vm.get_arg(i)) + "  ";
	}

	res += "\n";
	String& vyse_str = vm.make_string(res.c_str(), res.size());
	vm.print(vm, &vyse_str);

	return VYSE_NIL;
}

Value stdlib::input(VM& vm, int argc) {
	for (int i = 0; i < argc; ++i) {
		const Value& v = vm.get_arg(i);
		std::string str = value_to_string(v);
		String& vy_str = vm.make_string(str.c_str(), str.size());
		vm.print(vm, &vy_str);
	}

	if (!vm.read_line) return VYSE_NIL;
	char* chars = vm.read_line(vm);
	String* string = &vm.take_string(chars, strlen(chars));
	return VYSE_OBJECT(string);
}

Value stdlib::setproto(VM& vm, int argc) {
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

vyse::Value vyse::stdlib::getproto(VM& vm, int argc) {
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

Value stdlib::assert_(VM& vm, int argc) {
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

Value stdlib::import(VM& vm, int argc) {
	static constexpr const char* fname = "import";

	if (argc != 1) {
		cfn_error(vm, fname, "Expected 1 argument to 'import'.");
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ObjType::string, fname)) {
		return VYSE_NIL;
	}

	Value vloaders = vm.get_global(VMLoadersName);
	if (not VYSE_IS_LIST(vloaders)) {
		cfn_error(vm, fname, "Global loader list ('__loaders__') not found.");
		return VYSE_NIL;
	}

	Value mod_name = vm.get_arg(0);
	const List& loaders = *VYSE_AS_LIST(vloaders);
	for (uint i = 0; i < loaders.length(); ++i) {
		Value loader = loaders[i];
		/// Skip any non-callable loaders
		/// TODO: Objects with an overloaded '__call' should be used as loaders.
		if (not(VYSE_IS_CLOSURE(loader) or VYSE_IS_CCLOSURE(loader))) continue;

		vm.m_stack.push(loader);
		vm.m_stack.push(mod_name);

		bool ok = vm.call(1);
		if (not ok) return VYSE_NIL;

		Value result = vm.m_stack.pop();

		if (not VYSE_IS_NIL(result)) {
			Value module_cache = vm.get_global(ModuleCacheName);
			if (VYSE_IS_TABLE(module_cache)) {
				VYSE_AS_TABLE(module_cache)->set(mod_name, result);
			}

			return result;
		}
	}

	cfn_error(vm, fname,
			  kt::format_str("Cannot find module '{}'", VYSE_AS_STRING(mod_name)->c_str()));

	return VYSE_NIL;
}