#pragma once

#include "common.hpp"

namespace snap {

///  IMPORTANT: after making any changes to this enum, make sure
///  relevant changes are made in the 4 variables below, and in the `op_strs`
///  array in the file debug.cpp.
enum class Opcode : u8 {
	// opcodes with a constant operand
	load_const,
	// reads the next opcode as an index
	// then attempts to get `constant_pool[index]`
	// field of TOS.
	index_fast,
	// Uses the next opcode as an index to load a string `field`
	// and sets PEEK(1)[field] = TOS
	table_set_fast,

	// opcodes with one operand
	set_var,
	get_var,
	set_upval,
	get_upval,
	make_func,
	call_func,

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
	negate,
	lnot,
	load_nil,
	close_upval,
	return_val,
	/// Set a table's key to some value.
	/// Table, Key and Value are expected
	/// to be in this order: [<table>, <key>, <value>]
	table_set,
	/// same as table_set, but here the value at stack depth
	/// 3 is guaranteed to be a table, hence no runtime check
	/// is required.
	table_set_safe,

	// assuming TOS as the field
	// and PEEK(1) as the table/array,
	// push field TOS of table PEEK(1)
	// onto the stack, popping TOS.
	index,

	// creates a new table and pushes it on top of the
	// stack.
	new_table,

	// opcodes with 2 operands
	jmp,
	jmp_if_false_or_pop,
	jmp_if_true_or_pop,
	pop_jmp_if_false,
	op_count
};

/// numerically lowest opcode that takes no operands
constexpr auto Op_0_operands_start = Opcode::pop;
/// numerically highest opcode that takes no operands
constexpr auto Op_0_operands_end = Opcode::new_table;

constexpr auto Op_const_start = Opcode::load_const;
constexpr auto Op_const_end = Opcode::index_fast;

/// numerically lowest opcode that takes one operand
constexpr auto Op_1_operands_start = Opcode::set_var;
/// numerically highest opcode that takes one operand
constexpr auto Op_1_operands_end = Opcode::call_func;

constexpr auto Op_2_operands_start = Opcode::jmp;
constexpr auto Op_2_operands_end = Opcode::pop_jmp_if_false;

} // namespace snap