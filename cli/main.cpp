#include <iostream>
#include <string>
#include <vm.hpp>

using namespace vy;

static void repl() {
	auto read_code = []() -> std::string {
		std::string code;
		std::getline(std::cin, code);
		return code;
	};

	auto exit_fn = [](VM&, int) -> Value { exit(0); };

	VM vm;
	vm.load_stdlib();
	auto ccl = vm.make<CClosure>(exit_fn);
	vm.set_global("exit", VYSE_OBJECT(&ccl));
	while (true) {
		std::cout << "-> ";
		std::string code = read_code();
		vm.runcode(code);
	}
}

static void execfile(const char* filepath) {
	VM vm;
	vm.load_stdlib();
	vm.runfile(filepath);
}

static void info() {
	printf("The Vyse Programming Language. v0.0.1 Pre-alpha .\n");
	printf("Usage: vy <filename>\n");
}

int main(int const argc, char** const argv) {
	if (argc == 1) {
		repl();
	} else if (argc == 2) {
		execfile(argv[1]);
	} else {
		info();
	}

	return 0;
}
