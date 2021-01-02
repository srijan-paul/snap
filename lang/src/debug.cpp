#include <array>
#include <cstdio>
#include <debug.hpp>
#include <iostream>

namespace snap {
using Op = Opcode;

#define PRINT_VAL(v) (printf("%s", ((v).name_str().c_str())))

// clang-format off
static constexpr std::array<const char*, static_cast<size_t>(Op::op_count)> op_strs = {
	"load_const", "set_var", "get_var",
	"pop", "add", "concat",
	"sub", "mult", "mod", 
	"div", "eq", "neq", 
	"lshift", "rshift", "band",
	"bor","nil", "return", "jmp"
};
// clang-format on

const char* const op2s(Op op) {
	return op_strs[static_cast<u32>(op)];
}

static size_t constant_instr(const Block& block, Op op, size_t index) {
	const auto const_index = (u8)(block.code[index + 1]);
	const Value v = block.constant_pool[const_index];
	std::printf("%-4d	%-4zu  %s  (%d) ", block.lines[index], index, op2s(op), const_index);
	PRINT_VAL(v);
	printf("\n");
	return 2;
}

static size_t simple_instr(const Block& block, Op op, size_t index) {
	std::printf("%-4d	%-4zu  %s\n", block.lines[index], index, op2s(op));
	return 1;
}

static size_t instr_single_operand(const Block& block, size_t index) {
	Op op = block.code[index];
	Op operand = block.code[index + 1];
	std::printf("%-4d	%-4zu  %s  %d\n", block.lines[index], index, op2s(op),
				static_cast<int>(operand));
	return 2;
}

size_t disassemble_instr(const Block& block, Op op, size_t offset) {
	if (op >= Op_0_operands_start && op <= Op_0_operands_end) {
		return simple_instr(block, op, offset);
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