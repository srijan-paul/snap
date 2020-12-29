#include "test_utils.hpp"
#include <opcode.hpp>
#include <debug.hpp>

using namespace snap;
using Op = snap::Opcode;


void block_test() {
	std::printf("--- block test ---\n");

	Block b;
	const u8 index = b.add_value(SNAP_FLOAT_VAL(1.5));
	b.add_instruction(Op::load_const);
	b.add_num(index);
	b.add_instruction(Op::pop);

	disassemble_block(b);

	std::printf("--- /block test ---\n\n");
}

static void compiler_test() {
	std::printf("--- Compiler test ---\n");

	// expressions
	print_disassembly("1 + 2 - 3 * 4");

	// variable declaration
	print_disassembly("let a = 1");
	print_disassembly("let a = 1, b = 2 * 2");

	// variable access
	print_disassembly("let a = 1;"
					  "let b = a + 1;");

	std::printf("--- / Compiler test ---\n");
}

int main() {
    block_test();
	compiler_test();
	return 0;
}