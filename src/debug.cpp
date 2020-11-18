#include "debug.hpp"
#include <array>
#include <cstdio>
#include <iostream>

namespace snap {
using Op = Opcode;

#define op2s(op) op_strs[(size_t)op]

static constexpr std::array op_strs = {"push", "pop", "add", "sub", "mult", "mod"};

static size_t constant_instr(const Block& block, Op op, size_t index) {
	const auto const_index = (u8)(block.code[index + 1]);
	const Value v = block.constant_pool[const_index];
	std::printf("%-4zu  %s  (%d) %f\n", index, op2s(op), const_index, v);
	return 2;
}

static size_t simple_instr(Op op, size_t index) {
	std::printf("%-4zu  %s\n", index, op2s(op));
	return 1;
}

static size_t disassemble_instr(const Block& block, Op op, size_t offset) {
	if (op >= Op_0_operands_start && op <= Op_0_operands_end) {
		return simple_instr(op, offset);
	} else if (op >= Op_1_operands_start && op <= Op_1_operands_end) {
		return constant_instr(block, op, offset);
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