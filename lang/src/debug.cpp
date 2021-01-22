#include <array>
#include <cstdio>
#include <debug.hpp>
#include <iostream>
#include <value.hpp>

namespace snap {
using Op = Opcode;

#define PRINT_VAL(v) (printf("%s", ((v).name_str().c_str())))

static constexpr std::array<const char*, static_cast<std::size_t>(Op::op_count)> op_strs = {
	"load_const",

	"set_var",
	"get_var",
	"set_upval",
	"get_upval",
	"make_func",
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

	"jmp",
	"jmp_if_false_or_pop",
	"jmp_if_true_or_pop",
	"pop_jmp_if_false",
};

const char* op2s(Op op) {
	return op_strs[static_cast<u32>(op)];
}

static std::size_t constant_instr(const Block& block, Op op, std::size_t index) {
	const auto const_index = (u8)(block.code[index + 1]);
	const Value v = block.constant_pool[const_index];
	std::printf("%04d	%-4zu  %-22s (%d) ", block.lines[index], index, op2s(op), const_index);
	PRINT_VAL(v);
	printf("\n");
	return 2;
}

static std::size_t simple_instr(const Block& block, Op op, std::size_t index) {
	if (block.lines[index] == block.lines[index - 1])
		std::printf("    	");
	else
		std::printf("%04d	", block.lines[index]);
	std::printf("%-4zu  %-22s\n", index, op2s(op));
	return 1;
}

static std::size_t instr_single_operand(const Block& block, std::size_t index) {
	Op op = block.code[index];
	Op operand = block.code[index + 1];

	if (block.lines[index] == block.lines[index - 1])
		std::printf("    	");
	else
		std::printf("%04d	", block.lines[index]);
	std::printf("%-4zu  %-22s  %d\n", index, op2s(op), static_cast<int>(operand));
	return 2;
}

static std::size_t instr_two_operand(const Block& block, std::size_t index) {
	Op op = block.code[index];

	u8 a = (u8)(block.code[index + 1]);
	u8 b = (u8)(block.code[index + 2]);

	u16 distance = (u16)((a << 8) | b);

	if (block.lines[index] == block.lines[index - 1])
		std::printf("    	");
	else
		std::printf("%04d	", block.lines[index]);
	std::printf("%-4zu  %-22s  %d  (%zu)\n", index, op2s(op), distance, index + distance + 3);

	return 3;
}

std::size_t disassemble_instr(const Block& block, Op op, std::size_t offset) {

	if (op == Op::make_func) {
		int old_loc = offset;

		std::printf("%04d	", block.lines[offset]);
		std::printf("%-4zu  %-22s  ", offset++, op2s(op));
		print_value(block.constant_pool[(size_t)block.code[offset]]);
		std::printf("\n");
		
		u8 num_upvals = static_cast<u8>(block.code[++offset]);
		for (int i = 0; i < num_upvals; ++i) {
			bool is_local = static_cast<bool>(block.code[offset++]);
			if (is_local) {
				int idx = static_cast<int>(block.code[offset++]);
				std::printf("        %-4zu  %-22s  %s %d\n", offset - 1, " ", is_local ? "local" : "upvalue",
							idx);
			}
		}
		return offset - old_loc + 1;
	}

	if (op >= Op_0_operands_start && op <= Op_0_operands_end) {
		return simple_instr(block, op, offset);
	} else if (op >= Op_const_start && op <= Op_const_end) {
		return constant_instr(block, op, offset);
	} else if (op >= Op_1_operands_start && op <= Op_1_operands_end) {
		return instr_single_operand(block, offset);
	} else if (op >= Op_2_operands_start && op <= Op_2_operands_end) {
		return instr_two_operand(block, offset);
	}
	return 1;
}

void disassemble_block(const char* name, const Block& block) {
	std::printf("< Disassembly: %s >\n", name);
	for (std::size_t i = 0; i < block.code.size();) {
		i += disassemble_instr(block, block.code[i], i);
	}
}

#undef op2s

} // namespace snap