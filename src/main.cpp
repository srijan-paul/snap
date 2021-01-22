#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
			fn outer() {
				let x = "x"
				fn inner() {
					return x .. "-woot"
				}
				return inner()
			}
			return outer()
	)";

	VM vm{&code};
	vm.interpret();

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}