#pragma once

#include "common.hpp"
#include <array>


namespace snap {

///  IMPORTANT: after making any changes to this enum, make sure
///  relevant changes are made in the 4 variables below, and in the `op_strs`
///  array in the file debug.cpp.
enum class Opcode : u8 {
#define OP(name, _, __) name
#include <opcodex>
#undef OP
};

constexpr std::array<int, size_t(Opcode::no_op)> StackEffects = {

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