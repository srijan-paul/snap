#include <iostream>
#include <string>
#include <vm.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace vy;
#ifdef _WIN32
void clear() {
	COORD topLeft = {0, 0};
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
	FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
							   screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
	SetConsoleCursorPosition(console, topLeft);
}
#else
void clear() {
	// CSI[2J clears screen, CSI[H moves the cursor to top-left corner
	std::cout << "\x1B[2J\x1B[H";
}
#endif

static void repl() {
	auto read_code = []() {
		std::string code;
		std::getline(std::cin, code);
		return code;
	};

	auto exit_fn = [](VM&, int) -> Value { exit(0); };
	auto clear_screen_fn = [](VM&, int) -> Value {
		clear();
		return VYSE_NIL;
	};

	VM vm;
	vm.load_stdlib();

	auto exit_ccl = vm.make<CClosure>(exit_fn);
	vm.set_global("exit", VYSE_OBJECT(&exit_ccl));

	auto clear_ccl = vm.make<CClosure>(clear_screen_fn);
	vm.set_global("clear", VYSE_OBJECT(&clear_ccl));

	while (true) {
		std::cout << ">> ";
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
