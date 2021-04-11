#pragma once
#include "common.hpp"
#include "forward.hpp"
#include "opcode.hpp"

#include <vector>

namespace snap {

struct Block {
	std::vector<Opcode> code;
	std::vector<Value> constant_pool;
	std::vector<u32> lines;

	size_t add_instruction(Opcode i, u32 line);
	size_t add_num(u8 i, u32 line);
	size_t add_value(Value value);
	size_t op_count() const noexcept {
		return code.size();
	}
};

} // namespace snap