#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		fn make_barker() {
			return fn() {
				return "bow bow!"
			}
		}

		const barker = make_barker()
		return barker()
	)";

	VM vm{&code};
	vm.interpret();

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}