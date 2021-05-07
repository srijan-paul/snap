// Note: This file is an XMacro. ()
// The purpose of this kind of file is to keep the
// name-arity-stack effect of the opcodes all organized
// in a single place that will be easy to gloss over and edit.

// The way this file is used is simple.
// We first define a macro with the name `OP` and then include
// this file right under, expanding the macro with all the opcode
// data.
// I got this idea from here: https://github.com/wren-lang/wren/blob/main/src/vm/wren_opcodes.h
// for usage, see debug.cpp, opcode.hpp and compiler.cpp


// OP(name, arity, stack_effect),
OP(load_const, 1, 1),
OP(get_global, 1, 1),
OP(set_global, 1, -1),
OP(table_get, 1, 0),
OP(table_set, 1, -1),
OP(table_get_no_pop, 1, 1),
OP(set_var, 1, -1),
OP(get_var, 1, 1),
OP(set_upval, 1, -1),
OP(get_upval, 1, 1),
OP(make_func, -1, 1), /* special arity */
OP(prep_method_call, 1, 1),

// Note that calling function pushes a new call 
// frame onto the stack, therefore it does not count
// as incrementing the stack size of the *current*
// closure.
OP(call_func, 1, 0), /* special stack effect */

OP(pop, 0, -1),

// binary ops
OP(add, 0, -1),
OP(concat, 0, -1),
OP(sub, 0, -1),
OP(mult, 0, -1),
OP(mod, 0, -1),
OP(div, 0, -1),
OP(exp, 0, -1),
OP(eq, 0, -1),
OP(neq, 0, -1),
OP(lshift, 0, -1),
OP(rshift, 0, -1),
OP(band, 0, -1),
OP(bxor, 0, -1),
OP(bor, 0, -1),
OP(gt, 0, -1),
OP(lt, 0, -1),
OP(gte, 0, -1),
OP(lte, 0, -1),

// unary ops
OP(negate, 0, 0),
OP(len, 0, 0),
OP(bnot, 0, 0),
OP(lnot, 0, 0),


OP(load_nil, 0, 1),
OP(close_upval, 0, -1),
OP(return_val, 0, 0), /* special stack effect */


// table indexing
OP(new_table, 0, 1),
OP(index_set, 0, -2),
OP(table_add_field, 0, -2),
OP(index, 0, -1),
OP(index_no_pop, 0, 1),

OP(jmp, 2, 0),
OP(jmp_back, 2, 0),
OP(jmp_if_false_or_pop, 2, 0), /* -1 if TOS is truthy */
OP(jmp_if_true_or_pop, 2, 0),  /* -1 if TOS is falsy */
OP(pop_jmp_if_false, 2, -1),


/// TODO: explain stuff
OP(for_prep, 2, 1),

/// Operands:  A, B (Jump distance)
/// COUNTER  = TOS[-4]
/// LOOP_VAR = TOS[-1]
/// STEP     = TOS[-2]
/// LIMIT    = TOS[-3]
/// COUNTER += STEP
/// LOOP_VAR = COUNTER
/// if COUNTER < LIMIT ->
///   ip -= AB
OP(for_loop, 2, 0),

OP(no_op, -1, 0),