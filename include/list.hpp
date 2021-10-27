#pragma once
#include "util.hpp"
#include "value.hpp"

namespace vy {

class List final : public Obj {
  public:
	static constexpr size_t DefaultCapacity = 8;
	static constexpr uint GrowthFactor = 2;

	List() : Obj(ObjType::list){};
	List(size_t mincap);

	~List();

	/// @brief appends an item to the end of the array.
	/// This increments the current item count by 1.
	void append(Value value);

	/// @brief pops an element from the end of the
	/// array and returns it. If the array is empty,
	/// returns nil.
	Value pop() noexcept;

	/// @brief returns the number of items
	/// currently active in the array.
	inline size_t length() const noexcept {
		return m_num_entries;
	}

	inline size_t cap() const noexcept {
		return m_capacity;
	}

	/// @brief returns true if `index` is a valid key for this list.
	inline bool in_range(number index) const noexcept {
		return index >= 0 and index < m_num_entries;
	}

	virtual size_t size() const noexcept override {
		return sizeof(List) + m_capacity * sizeof(Value);
	}

	/// @brief Makes sure there is space for at least 1
	/// more insertion. Ths may grow the list.
	void ensure_capacity();

	Value at(size_t index) const noexcept {
		return m_values[index];
	}

	Value& operator[](size_t index) {
		VYSE_ASSERT(index < m_num_entries, "List index out of range!");
		return m_values[index];
	}

	const Value& operator[](size_t index) const {
		VYSE_ASSERT(index < m_num_entries, "List index out of range!");
		return m_values[index];
	}

  private:
	size_t m_capacity = DefaultCapacity;
	size_t m_num_entries = 0;
	Value* m_values = (Value*)malloc(DefaultCapacity * sizeof(Value));

	virtual void trace(GC& gc) noexcept override;
};

} // namespace vy
