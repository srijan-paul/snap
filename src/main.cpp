#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		fn x() {
			let a = 1
			let b = 2
			fn y() {
				let c = 3
				fn z() {
					return a + b + c;
				}
				return z
			}
			return y
		}
		return x()()()
	)";

	VM vm{&code};
	vm.interpret();

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}