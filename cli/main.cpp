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

using namespace vyse;

ExitCode runcode(std::string const& code) {
	VM vm;
	vm.load_stdlib();
	ExitCode ec = vm.runcode(code);

	return ec;
}

std::optional<std::string> readfile(const char* filepath) {
	std::ifstream file(filepath);
	if (!file.good()) return std::nullopt;
	std::ostringstream stream;
	stream << file.rdbuf();
	return stream.str();
}

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("The Vyse Programming Language. v0.0.1 Pre-alpha .\n");
		printf("Usage: vy <filename>\n");
		return 0;
	}

	const char* filepath = argv[1];
	if (auto code = readfile(filepath)) {
		runcode(*code);
	} else {
		printf("Could not read file '%s'\n", filepath);
	}

	return 0;
}