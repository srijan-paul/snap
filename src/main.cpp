#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vm.hpp>

using namespace snap;

int main() {
	// Temporarily we use this entry point to run
	// a file main.snp, so we can use that as a
	// 'scratchpad' of sorts to test some code.
	const char* filepath = "../src/main.snp";
	std::ifstream file(filepath);
	assert(file.good() && "File doesn't exist");

	std::ostringstream stream;
	stream << file.rdbuf();
	std::string code(stream.str());

	VM vm{&code};
	vm.interpret();

	std::cout << "VM returned: ";
	print_value(vm.return_value);
	std::cout << std::endl;

	std::cout << "Snap programming language."
			  << "\n";
	return 0;
}