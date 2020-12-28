#include <iostream>
#include <vm.hpp>

int main() {
	std::string test = "let a = 1;";
	snap::VM vm{&test};
	std::printf("Snap programming language.\n");
	return 0;
}