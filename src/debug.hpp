#pragma once
#include "block.hpp"

namespace snap {
void disassemble_block(const Block& block);
const char* const op2s(Opcode op);
}; // namespace snap