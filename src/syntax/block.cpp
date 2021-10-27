#include <block.hpp>
#include <common.hpp>
#include <value.hpp>

namespace vy {

size_t Block::add_instruction(Opcode i, u32 line) {
	code.push_back(i);
	lines.push_back(line);
	return code.size() - 1;
}

size_t Block::add_num(u8 i, u32 line) {
	code.emplace_back(static_cast<Opcode>(i));
	lines.push_back(line);
	return code.size() - 1;
}

size_t Block::add_value(Value value) {
	constant_pool.push_back(value);
	return constant_pool.size() - 1;
}

} // namespace vyse
