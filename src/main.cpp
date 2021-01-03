#include "debug.hpp"
#include "scanner.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		const a = nil && 1 && 3 && 5;
		const foo = false || false || 123;
		const b = 10;
	)";

	VM vm{&code};
	vm.interpret();

	std:: cout << "Snap programming language." << "\n";

	return 0;
}