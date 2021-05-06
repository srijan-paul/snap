#include "test_utils.hpp"
#include <debug.hpp>
#include <opcode.hpp>

using namespace vyse;
using Op = vyse::Opcode;

void block_test() {
	std::printf("--- block test ---\n");

	Block b;
	const u8 index = b.add_value(VYSE_NUM(1.5));
	b.add_instruction(Op::load_const, 1);
	b.add_instruction(static_cast<Op>(index), 1);
	b.add_instruction(Op::pop, 1);

	disassemble_block("test block", b);

	std::printf("--- /block test ---\n\n");
}

static void compiler_test() {
	std::printf("--- Compiler test ---\n");

	// // expressions
	print_disassembly("let a = 1 + 2 - 3 * 4");

	// // variable declaration
	print_disassembly("let a = 1");
	print_disassembly("let a = 1, b = 2 * 2");

	// // variable access
	print_disassembly("let a = 1;"
					  "let b = a + 1;");

	print_disassembly("let s = 'this is a string'");

	print_disassembly(R"(
		let T = {a : 1} 
	)");

	// print_disassembly(R"(
	// 	const tbl = {
	// 		[123 + 4]: "abc" .. "def"
	// 	}
	// )");

	std::printf("--- / Compiler test ---\n");
}

int main() {
	block_test();
	compiler_test();
	return 0;
}