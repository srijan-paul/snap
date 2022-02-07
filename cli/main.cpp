#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vm.hpp>
#include <optional>

using namespace vy;

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("The Vyse Programming Language. v0.0.1 Pre-alpha .\n");
		printf("Usage: vy <filename>\n");
		return 0;
	}

	const char* filepath = argv[1];
	VM vm; vm.load_stdlib();
	vm.runfile(filepath);

	return 0;
}