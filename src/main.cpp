#include <iostream>
#include <vm.hpp>

int main() {
	std::string test = R"(let foo = 2;
						let bar = 5;
						let baz = bar / 0;)";
	snap::VM vm{&test};
	auto code = vm.interpret();
	std::printf("Snap programming language.\n");
	return 0;
}