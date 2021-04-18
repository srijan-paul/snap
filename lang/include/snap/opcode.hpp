#pragma once

#include "common.hpp"

namespace snap {

///  IMPORTANT: after making any changes to this enum, make sure
///  relevant changes are made in the 4 variables below, and in the `op_strs`
///  array in the file debug.cpp.
enum class Opcode : u8 {
	// opcodes with a constant operand

	/// Operands: Idx
	/// Stack Effect: 1
	/// Loads the constant present at index `Idx` in the
	/// constant pool on top of the stack.
	load_const,

	/// Operands: Idx
	/// Stack effect: 1
	/// Pushes the global present at index [Idx] in the
	/// constant pool to the top of the stack.
	get_global,

	/// Operands: Idx,
	/// Stack Effect: 0
	/// Sets the global variable having the name constant_pool[Idx]
	/// to the value present on top of the stack.
	set_global,

	/// Operands: Idx
	/// Stack Effect: 0
	/// Reads the next opcode as an index
	/// then attempts to get `constant_pool[index]`
	/// field of TOS. It assumes that the constant_pool[index]
	/// is a string value and hence doesn't perform any runtime
	/// checks.
	table_get,

	/// Operands: FieldIdx (index of the field in the constant pool)
	/// Stack Effect: -1
	/// Assuming the value on top of the stack as `table`,
	/// Uses the next opcode as an index to load a string `field`
	/// and sets `table[field] = TOS. Pops table and replaces it with TOS.
	table_set,

	/// Operands: FieldIdx (index of the field in the constant pool)
	/// Stack Effect: 0
	/// Same as `table_get`, but keeps both the table
	/// and the value received on top of the stack.
	table_get_no_pop,

	// Opcodes with one operand

	/// Operands: Idx
	/// Stack Effect: 0
	/// Sets the variable at stack slot `index`
	/// to the value present on top of the stack.
	set_var,

	/// Operands: Idx
	/// Stack Effect: 1
	/// Pushes the value at stack slot `Idx` on top
	/// of the stack.
	get_var,

	/// Operands: Idx
	/// Stack Effect: 0
	/// Sets the upvalue at index `Idx` in the
	/// current function's upvalue list to the
	/// value that is present on top of the stack.
	set_upval,

	/// Operands: Idx
	/// Stack Effect: 1
	/// Pushes the upvalue in the current function's
	/// upvalue list at index `Idx` on top of the stack.
	get_upval,

	/// Operands: NumUpvals, ... (varargs)
	/// Gets a function object from the
	/// constant pool and pushes it onto the stack.
	/// This Opcode is special in that it takes a
	/// variable number of arguments, the first parameter (N)
	/// specifies the number of upvalues that this function
	/// has, followed by a series of opcode pairs, The first
	/// Op specifies if the upvalue is a local variable currently
	/// on the stack.
	/// If that Op is 1, then the next op contains the index of the
	/// local in the stack that is captured.
	/// If the Op is 0, then the next op contains the index of the
	/// Upvalue in the current function's `m_upvalues` list.
	make_func,

	/// Operands: Key
	/// Stack Effect: 1
	/// table <- TOS
	/// TOS   <- table[Key]
	/// PUSH(table)
	prep_method_call,

	/// Operands : NumArgs
	/// Stack Effect: -NumArgs
	/// (Pops everything upto the function off the
	/// stack and pushes the return value)
	/// Calls the function object a stack depth
	/// NumArgs + 1, taking everything above from
	call_func,

	// opcodes with no operands

	/// Stack Effect: -1
	/// Pops a value off the stack and discards it.
	pop,

	/// Stack Effect: -1
	/// Pops off two values from the stack, and pushes
	/// the sum back on.
	add,

	/// Stack Effect: -1
	/// TOS = string.concat(TOS1..TOS)
	concat,

	/// Stack Effect: -1
	/// TOS = TOS1 - TOS
	sub,

	/// Operands: 0
	/// Stack Effect: -1
	/// TOS = TOS * TOS1
	mult,

	/// Operands: 0
	/// Stack Effect: -1
	/// TOS = TOS1 % TOS
	mod,

	/// Stack Effect: -1
	/// TOS = TOS1 / TOS
	div,

	/// Stack Effect: -1
	/// TOS = TOS == TOS1
	eq,

	/// Stack Effect: -1
	/// TOS = TOS != TOS1
	neq,

	/// Stack Effect: -1
	/// TOS = TOS1 << TOS
	lshift,

	/// Stack Effect: -1
	/// TOS = TOS1 >> TOS
	rshift,

	/// Stack Effect: -1
	/// TOS = TOS1 & TOS
	band,

	/// Stack Effect: -1
	/// TOS = TOS1 | TOS
	bor,

	/// Stack Effect: -1
	/// TOS = TOS1 > TOS
	gt,

	/// Stack Effect: -1
	/// TOS = TOS1 < TOS
	lt,

	/// Stack Effect: -1
	/// TOS = TOS1 >= TOS
	gte,

	/// Stack Effect: -1
	/// TOS = TOS1 <= TOS
	lte,

	/// Stack Effect: 0
	/// TOS = -TOS
	negate,

	/// Stack Effect: 0
	/// TOS = !TOS
	lnot,

	/// Stack Effect: 1
	/// PUSH(NIL)
	load_nil,

	/// Stack Effect: 0
	/// Closes the upvalue present at TOS
	close_upval,

	/// Returns from the current CallFrame.
	/// Popping it off the call stack.
	return_val,

	/// Stack Effect: 1
	/// Creates a new table and pushes it on top of the
	/// stack.
	new_table,

	/// Stack Effect: -2
	/// Set a table's key to some value.
	/// Table, Key and Value are expected
	/// to be in this order: [<table>, <key>, <value>]
	index_set,

	/// Stack Effect: -2
	/// same as index_set, but here the value at stack depth
	/// 3 is guaranteed to be a table, hence no runtime check
	/// is required. The table isn't popped off the stack.
	/// This instruction is used solely during the construction
	/// of a table.
	table_add_field,

	/// Stack Effect: 0
	/// assuming TOS as the field
	/// and PEEK(1) as the table/array,
	/// push field TOS of table PEEK(1)
	/// onto the stack, popping TOS and TOS1.
	/// `key = pop(); TOS = TOS[key]`.
	index,

	/// Stack Effect: 1
	/// same as `index`, but doesn't
	/// pop the table or the key, instead pushes
	/// the value on top. So we're left with the stack
	/// looking like [table, key, table[key]].
	index_no_pop,

	// opcodes with 2 operands

	/// Operands: A, B
	/// Jumps forward AB bytes in code,
	/// A and B when stitched together form
	/// a short int.
	jmp,

	/// Operands: A B
	/// Stack Effect: 0
	/// Jump the instruction pointer backwards
	/// by AB. (ip = ip - AB)
	jmp_back,

	/// Operands: A, B
	/// Stack Effect: 0
	/// If the value on top of the stack
	/// is falsy, then jump forward AB
	/// instructions.
	jmp_if_false_or_pop,

	/// Operands: A, B
	/// Stack Effect: 0
	/// If the Value on top of the
	/// stack is truthy, then jump
	/// forward AB insructions.
	jmp_if_true_or_pop,

	/// Operands: A, B
	/// Stack Effect: -1
	/// Pop the value at TOS.
	/// If the value discarded is falsy then jump
	/// forward AB instructios.
	pop_jmp_if_false,
	no_op
};

/// numerically lowest opcode that takes no operands
constexpr auto Op_0_operands_start = Opcode::pop;
/// numerically highest opcode that takes no operands
constexpr auto Op_0_operands_end = Opcode::index_no_pop;

constexpr auto Op_const_start = Opcode::load_const;
constexpr auto Op_const_end = Opcode::table_get_no_pop;

/// numerically lowest opcode that takes one operand
constexpr auto Op_1_operands_start = Opcode::set_var;
/// numerically highest opcode that takes one operand
constexpr auto Op_1_operands_end = Opcode::call_func;

constexpr auto Op_2_operands_start = Opcode::jmp;
constexpr auto Op_2_operands_end = Opcode::pop_jmp_if_false;

} // namespace snap