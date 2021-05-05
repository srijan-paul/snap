#pragma once
#include "common.hpp"
#include "value.hpp"
#include <cstddef>

namespace vyse {

/// Total wasted hours: 12 
class Stack final {
	friend VM;
	friend GC;
	VYSE_NO_COPY(Stack);
	VYSE_NO_MOVE(Stack);

public:
	static constexpr size_t InitialSize = 4;

	Stack() = default;

	/// @brief Pushes [value] on top of the stack.
	inline void push(Value value) noexcept {
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

	/// @brief saves the current top of the stack
	/// so it can be restored later
	void save_stack() noexcept {
		m_saved_top = top;
	}

	/// @brief restores the top of the stack
	/// by setting it back to where it was
	/// when [savestack] was last called.
	void restore_stack() noexcept {
		top = m_saved_top;
	}

	/// @brief Ensures that there are at least [num_slots]
	/// slots free past the top in the stack.
	/// @return returns true if the stack growht was successful
	/// and false if the program ran out of memory trying to
	/// grow the stack.
	bool ensure_cap(size_t num_slots) {
		std::ptrdiff_t count = top - m_values;
		size_t num_free_slots = m_size - count;

		/// TODO: update saved_top.
		if (num_slots > num_free_slots) {
			size_t diff = num_slots - num_free_slots;
			m_size += diff + 1;
			m_values = static_cast<Value*>(realloc(m_values, m_size * sizeof(Value)));
			if (m_values == nullptr) return false;
			top = m_values + count;
		}

		return true;
	}

private:
	Value* m_values = static_cast<Value*>(malloc(sizeof(Value) * InitialSize));

	/// @brief Points to the next free slot in the stack.
	Value* top = m_values;

	/// @brief The last saved location of [sp].
	/// This is used to restore the stack's
	/// top with [restore_stack
	Value* m_saved_top = m_values;
	size_t m_size = InitialSize;
};

}; // namespace vyse
