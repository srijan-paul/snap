#include "scanner.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		let a = 2
		let b = 5
		let c = b % 2
	)";

	VM vm{&code};
	vm.interpret();

	std::printf("Snap programming language.\n");
	return 0;
}