#pragma once
#include "common.hpp"
#include "value.hpp"
#include <cstddef>

namespace vyse {

/// Total wasted hours: 16
class Stack final {
	friend VM;
	friend GC;
	VYSE_NO_COPY(Stack);
	VYSE_NO_MOVE(Stack);

public:
	static constexpr size_t InitialSize = 128;

	Stack() = default;
	~Stack() noexcept {
		VYSE_ASSERT(values != nullptr, "stack is freed before destruction");
		free(values);
	}

	/// @brief Pushes [value] on top of the stack.
	inline void push(Value value) noexcept {
		VYSE_ASSERT(top < (values + size), "Top points past the end of stack.");
		*(top++) = value;
	}

	inline Value peek(u8 depth = 0) const {
		return *(top - 1 - depth);
	}

	/// @brief Pops a value from the stack and returns it
	inline Value pop() {
		return *(--top);
	}

	/// @brief Pop the topmost [n] values from the stack.
	/// @param n The number of values to pop.
	void popn(int n) {
		VYSE_ASSERT(n >= 0, "[n] must be >= 1");
		top -= n;
	}

	inline void clear() noexcept {
		top = values;
	}

private:
	Value* values = static_cast<Value*>(malloc(sizeof(Value) * InitialSize));

	/// @brief Points to the next free slot in the stack.
	Value* top = values;
	uint size = InitialSize;
};

} // namespace vyse
