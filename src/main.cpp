#include "debug.hpp"
#include "scanner.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		let a = 'a' .. 'b';
		let b = 'c' .. 'd';
		a = b .. b
		a = a .. '1'
	)";

	VM vm{&code};
	vm.interpret();

	std:: cout << "Snap programming language." << "\n";

	return 0;
}