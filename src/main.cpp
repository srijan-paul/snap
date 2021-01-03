#include "debug.hpp"
#include "scanner.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		const a = 1 && 2 && false && 4 && 5
	)";

	VM vm{&code};
	vm.interpret();

	std:: cout << "Snap programming language." << "\n";

	return 0;
}