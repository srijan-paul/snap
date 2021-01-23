#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		fn make_adder(x) {
			fn adder(y) {
				return x + y
			}
			return adder
		}
		let add10 =  make_adder(10)
		return add10(20)
	)";

	VM vm{&code};
	vm.interpret();

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}