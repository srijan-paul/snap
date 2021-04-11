#include "../str_format.hpp"
#include <std/base.hpp>
#include <vm.hpp>

/// TODO: benchmark and optimize this.
int snap::stdlib::print(VM& vm, int argc) {
	std::string res = "";
	for (int i = 0; i < argc; ++i) {
		res += value_to_string(vm.get_arg(i)) + "\t";
	}

	res += "\n";
	String& snap_str = vm.string(res.c_str(), res.size());
	vm.print(vm, &snap_str);

	vm.popn(argc); // pop all the arguments and the function off the stack.
	return 0;
}

int snap::stdlib::setmeta(VM& vm, int argc) {
	SNAP_ASSERT(argc == 2, "Incorrect number of arguments for function 'setmeta'.");
	Value& vtable = vm.get_arg(0);
	Value& vmeta = vm.get_arg(1);

	/// TODO: this error message can be abstracted away a bit.
	if (!SNAP_IS_TABLE(vtable)) {
		vm.runtime_error(kt::format_str("Bad argument #1 to 'setmeta'. Expected table, got {}.",
																		SNAP_TYPE_CSTR(vtable)));
		return 0;
	}

	if (!SNAP_IS_TABLE(vmeta)) {
		vm.runtime_error(kt::format_str("Bad argument #2 to 'setmeta'. Expected table, got {}.",
																		SNAP_TYPE_CSTR(vmeta)));
		return 0;
	}

	SNAP_AS_TABLE(vtable)->m_meta_table = SNAP_AS_TABLE(vmeta);
	vm.popn(argc);
	return 0;
}