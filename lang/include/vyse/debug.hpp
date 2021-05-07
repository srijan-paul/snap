#pragma once
#include "block.hpp"
#include <array>

namespace vyse {

void disassemble_block(const char* name, const Block& block);
size_t disassemble_instr(const Block& block, Opcode op, size_t offset);
const char* op2s(Opcode op);

// /// @brief op_arity[int(op)] returns the arity of [op] opcode.
// /// IMPORTANT: Note that the arity of Opcode::call_func has to
// /// be handled separately since it's a variadic opcode.
// constexpr std::array<int, size_t(Opcode::no_op) + 1> op_arity = {
//   #define OP(_, arity, __) arity
//   #include "x_opcode.hpp"
//   #undef OP
// };

} // namespace vyse
