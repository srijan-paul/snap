#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		const table = {
			a: 1,
			b: {
				a: 100,
				b: 2
			} 
		}
		table.b = table.b.a + 1 
		return 1 + table.b
	)";

	VM vm{&code};
	vm.interpret();

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}