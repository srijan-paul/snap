#pragma once

#include "common.hpp"

namespace snap {

enum class OpCode { Return = 0 };

u8 decode_op(Instruction i) {
	return (i & 0xff0000) >> 24;
}

} // namespace snap