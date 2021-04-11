#include "base.hpp"
#include "value.hpp"

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
