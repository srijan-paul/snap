#include "test_utils.hpp"

static void run_test() {
	std::printf("--- parser test ---\n");
	// expressions and precedence
	print_parsetree("1 + 2; 3 + 4;");
	print_parsetree("1 | 2 | 3 || 4");

	// variable declaration
	print_parsetree("let a = 1");
	print_parsetree("let a = 1, b = 2 * 3, c = 3 || 4");

	// variable access
	print_parsetree("let a = 1; a = a + 1");
	std::printf("--- /parser test ---\n");
}

int main() {
    run_test();
    return 0;
}