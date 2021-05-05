#include "common.hpp"
#include "value.hpp"

namespace vyse {

class Stack final {
	friend VM;
	friend GC;
	VYSE_NO_COPY(Stack);
	VYSE_NO_MOVE(Stack);

public:
	static constexpr size_t InitialSize = 128;

	Stack(){};

	/// @brief Pushes [value] on top of the stack.
	void push(Value value) noexcept {
		*(top++) = value;
	}

	inline Value peek(u8 depth = 0) const {
		return *(top - 1 - depth);
	}

	/// @brief Pops a value from the stack and returns it
	Value pop() {
		return *(--top);
	}

	/// @brief Pop the topmost [n] values from the stack.
	/// @param n The number of values to pop.
	void popn(int n) {
		VYSE_ASSERT(n >= 0, "[n] must be >= 1");
		top -= n;
	}

	/// @brief saves the current top
	/// of the stack so it can be restored
	/// later.
	void save_stack() noexcept {
		m_saved_top = top;
	}

	/// @brief restores the top of the stack
	/// by setting it back to where it was
	/// when [savestack] was last called.
	void restore_stack() noexcept {
		top = m_saved_top;
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