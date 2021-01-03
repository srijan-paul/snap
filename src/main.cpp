#include "debug.hpp"
#include "scanner.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		let a = 1;
		let b = 2;
		if a <= 1 {
			b = 5
		}
	)";

	VM vm{&code};
	vm.interpret();

	std:: cout << "Snap programming language." << "\n";

	return 0;
}