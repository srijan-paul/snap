#pragma once

#include "common.hpp"

namespace snap {

///  IMPORTANT: after making any changes to this enum, make sure
///  relevant changes are made in the 4 variables below, and in the `op_strs`
///  array in the file debug.cpp.
enum class Opcode : u8 {
	// opcodes with a constant operand
	load_const,

	// opcodes with one operand
	set_var,
	get_var,

	// opcodes with no operands
	pop,
	add,
	concat,
	sub,
	mult,
	mod,
	div,
	eq,
	neq,
	lshift,
	rshift,
	band,
	bor,
	gt,
	lt,
	gte,
	lte,
	load_nil,
	return_val,

	// opcodes with 2 operands
	jmp,
	jmp_if_false,
	pop_jmp_if_false,
	op_count
};

/// numerically lowest opcode that takes no operands
constexpr auto Op_0_operands_start = Opcode::pop;
/// numerically highest opcode that takes no operands
constexpr auto Op_0_operands_end = Opcode::return_val;

constexpr auto Op_const_start = Opcode::load_const;
constexpr auto Op_const_end = Opcode::load_const;

/// numerically lowest opcode that takes one operand
constexpr auto Op_1_operands_start = Opcode::set_var;
/// numerically highest opcode that takes one operand
constexpr auto Op_1_operands_end = Opcode::get_var;

constexpr auto Op_2_operands_start = Opcode::jmp;
constexpr auto Op_2_operands_end = Opcode::pop_jmp_if_false;

} // namespace snap