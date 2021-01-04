#pragma once
#include "common.hpp"
#include "opcode.hpp"

#include <vector>

namespace snap {

struct Block {
	std::vector<Opcode> code = {};
	std::vector<Value> constant_pool = {};
	std::vector<u32> lines = {};

	std::size_t add_instruction(Opcode i, u32 line);
	std::size_t add_num(u8 i);
	std::size_t add_value(Value value);
	std::size_t op_count();
};


} // namespace snap