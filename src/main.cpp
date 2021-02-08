#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		fn Node(a, b) {
			return  { data: a, next: b }
		}
		const head = Node(10 Node(20))
		return head.next.data
	)";

	VM vm{&code};
	vm.interpret();

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}