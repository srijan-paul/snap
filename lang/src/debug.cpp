#include <array>
#include <cstdio>
#include <debug.hpp>
#include <iostream>
#include <value.hpp>

namespace vyse {

using Op = Opcode;
using std::printf;

// To keep a record of the names of all opcodes in the right order
// we use the clever XMacro and the stringize operator (#).
static constexpr std::array op_strs = {
#define OP(name, _, __) #name
#include <x_opcode.hpp>
#undef OP
};

const char* op2s(Op op) {
	return op_strs[static_cast<u32>(op)];
}

static void print_line(const Block& block, size_t index) {
	if (index == 0 or block.lines[index] == block.lines[index - 1]) {
		printf("   |	");
	} else {
		printf("\n%04d	", block.lines[index]);
	}
}

static size_t constant_instr(const Block& block, Op op, size_t index) {
	const u8 const_index = u8(block.code[index + 1]);
	const Value v = block.constant_pool[const_index];
	print_line(block, index);
	printf("%-4zu  %-22s  %d\t(", index, op2s(op), const_index);
	print_value(v);
	printf(")\n");
	return 2;
}

static size_t simple_instr(const Block& block, Op op, size_t index) {
	print_line(block, index);
	printf("%-4zu  %-22s\n", index, op2s(op));
	return 1;
}

static size_t instr_single_operand(const Block& block, size_t index) {
	Op op = block.code[index];
	Op operand = block.code[index + 1];

	print_line(block, index);
	printf("%-4zu  %-22s  %d\n", index, op2s(op), static_cast<int>(operand));
	return 2;
}

static size_t instr_two_operand(const Block& block, size_t index) {
	Op op = block.code[index];

	u8 a = u8(block.code[index + 1]);
	u8 b = u8(block.code[index + 2]);

	u16 distance = u16((a << 8) | b);

	print_line(block, index);
	size_t op_index = op == Op::jmp_back ? (index + 3) - distance : (index + 3) + distance;
	printf("%-4zu  %-22s  %d (%zu)\n", index, op2s(op), distance, op_index);

	return 3;
}

size_t disassemble_instr(const Block& block, Op op, size_t offset) {

	if (op == Op::make_func) {
		int old_loc = offset;

		printf("%04d	", block.lines[offset]);
		printf("%-4zu  %-22s  ", offset++, op2s(op));
		print_value(block.constant_pool[(size_t)block.code[offset]]);
		printf("\n");

		u8 num_upvals = static_cast<u8>(block.code[++offset]);
		for (int i = 0; i < num_upvals; ++i) {
			bool is_local = static_cast<bool>(block.code[offset++]);
			if (is_local) {
				int idx = static_cast<int>(block.code[++offset]);
				printf("        %-4zu  %-22s  %s %d\n", offset - 1, " ", is_local ? "local" : "upvalue",
							 idx);
			}
		}
		return offset - old_loc + 1;
	}

	if (op >= Op_0_operands_start and op <= Op_0_operands_end) {
		return simple_instr(block, op, offset);
	} else if (op >= Op_const_start and op <= Op_const_end) {
		return constant_instr(block, op, offset);
	} else if (op >= Op_1_operands_start and op <= Op_1_operands_end) {
		return instr_single_operand(block, offset);
	} else if (op >= Op_2_operands_start and op <= Op_2_operands_end) {
		return instr_two_operand(block, offset);
	}
	// no op
	return instr_two_operand(block, offset);
}

void disassemble_block(const char* name, const Block& block) {
	printf("<%s>\n", name);
	for (size_t i = 0; i < block.code.size();) {
		i += disassemble_instr(block, block.code[i], i);
	}
}

#undef op2s

} // namespace vyse