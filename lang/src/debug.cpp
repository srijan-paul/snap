#include <array>
#include <cstdio>
#include <debug.hpp>
#include <iostream>
#include <value.hpp>

namespace snap {

using Op = Opcode;
using std::printf;

static constexpr std::array<const char*, size_t(Op::op_count)> op_strs = {
		"load_const",
		"get_global",
		"set_global",
		"table_get",
		"table_set",
		"table_get_no_pop",
		"set_var",
		"get_var",
		"set_upval",
		"get_upval",
		"make_func",
		"prep_method_call",
		"call_func",
		"pop",
		"add",
		"concat",
		"sub",
		"mult",
		"mod",
		"div",
		"eq",
		"neq",
		"lshift",
		"rshift",
		"band",
		"bor",
		"gt",
		"lt",
		"gte",
		"lte",
		"negate",
		"lnot",
		"load_nil",
		"close_upval",
		"return_val",
		"new_table",
		"index_set",
		"table_add_field",
		"index",
		"index_no_pop",
		"jmp",
		"jmp_back",
		"jmp_if_false_or_pop",
		"jmp_if_true_or_pop",
		"pop_jmp_if_false",
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
	size_t op_index = op == Op::jmp_back ? index - distance : index + distance + 3;
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
	return 1;
}

void disassemble_block(const char* name, const Block& block) {
	printf("<%s>\n", name);
	for (size_t i = 0; i < block.code.size();) {
		i += disassemble_instr(block, block.code[i], i);
	}
}

#undef op2s

} // namespace snap