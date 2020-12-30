#pragma once
#include "common.hpp"
#include "opcode.hpp"
#include "value.hpp"
#include <vector>

namespace snap {

struct Block {
	std::vector<Opcode> code = {};
	std::vector<Value> constant_pool = {};
	std::vector<u32> lines = {};

	size_t add_instruction(Opcode i, u32 line);
	size_t add_num(u8 i);
	size_t add_value(Value value);
};

} // namespace snap