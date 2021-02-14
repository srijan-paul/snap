#include <block.hpp>
#include <common.hpp>
#include <value.hpp>

namespace snap {

size_t Block::add_instruction(Opcode i, u32 line) {
	code.emplace_back(i);
	lines.emplace_back(line);
	return code.size() - 1;
}

size_t Block::add_num(u8 i, u32 line) {
	code.emplace_back(static_cast<Opcode>(i));
	lines.emplace_back(line);
	return code.size() - 1;
}

size_t Block::add_value(Value value) {
	constant_pool.emplace_back(value);
	return constant_pool.size() - 1;
}

std::size_t Block::op_count() {
	return code.size();
}

} // namespace snap