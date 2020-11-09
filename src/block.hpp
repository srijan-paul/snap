#include "common.hpp"
#include "value.hpp"
#include <vector>

namespace snap {

struct Block {
	std::vector<Instruction> code = {};
	std::vector<Value> constant_pool = {};

	size_t add_instruction(Instruction i) {
		code.push_back(i);
		return code.size() - 1;
	}

	size_t add_value(Value value) {
		constant_pool.push_back(value);
		return constant_pool.size() - 1;
	}
};

} // namespace snap