#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <cassert>
#include <iostream>
#include <vm.hpp>

using namespace snap;

int main() {
	std::string code = R"(
		fn Node(a, b){
			return fn(n) {
				if n == 0 return a
				return b
			}
		}

		fn data(node) {
			return node(0)
		}

		fn next(node) {
			return node(1)
		}

		const head = Node(10, Node(20))
		return data(next(head))
	)";

	VM vm{&code};
	vm.interpret();

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}