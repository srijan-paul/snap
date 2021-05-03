#pragma once
#include "block.hpp"

namespace vyse {

void disassemble_block(const char* name, const Block& block);
size_t disassemble_instr(const Block& block, Opcode op, size_t offset);
const char* op2s(Opcode op);

} // namespace vyse
