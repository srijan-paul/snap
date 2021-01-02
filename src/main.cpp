#include "debug.hpp"
#include "scanner.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		{
			let a = 1;
			let b = 2;
			{let c = a + 1;}
		}
		let a = b;
	)";

	VM vm{&code};
	vm.interpret();

	// std::printf("Snap programming language.\n");
	return 0;
}