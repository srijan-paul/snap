#include "debug.hpp"
#include <array>
#include <cstdio>
#include <iostream>

namespace snap {
using Op = Opcode;

#define PRINT_VAL(v) (printf("%s", ((v).name_str().c_str())))

// clang-format off
static constexpr std::array op_strs = {
	"load_const",
	"set_var", "get_var",
	"pop", "add", "sub", "mult", "mod", "div", "nil", "return_val"
};
// clang-format on

const char* const op2s(Op op) {
	return op_strs[(u32)op];
}

static size_t constant_instr(const Block& block, Op op, size_t index) {
	const auto const_index = (u8)(block.code[index + 1]);
	const Value v = block.constant_pool[const_index];
	std::printf("%-4zu  %s  (%d) ", index, op2s(op), const_index);
	PRINT_VAL(v);
	printf("\n");
	return 2;
}

static size_t simple_instr(Op op, size_t index) {
	std::printf("%-4zu  %s\n", index, op2s(op));
	return 1;
}

static size_t instr_single_operand(const Block& block, size_t index) {
	Op op = block.code[index];
	Op operand = block.code[index + 1];
	std::printf("%-4zu  %s  %d\n", index, op2s(op), (int)(operand));
	return 2;
}

static size_t disassemble_instr(const Block& block, Op op, size_t offset) {
	if (op >= Op_0_operands_start && op <= Op_0_operands_end) {
		return simple_instr(op, offset);
	} else if (op >= Op_const_start && op <= Op_const_end) {
		return constant_instr(block, op, offset);
	} else if (op >= Op_1_operands_start && op <= Op_1_operands_end) {
		return instr_single_operand(block, offset);
	}

	return 1;
}

void disassemble_block(const Block& block) {
	for (size_t i = 0; i < block.code.size();) {
		i += disassemble_instr(block, block.code[i], i);
	}
}

#undef op2s

} // namespace snap