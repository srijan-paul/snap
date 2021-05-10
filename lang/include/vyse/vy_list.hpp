#include "value.hpp"

namespace vyse {

class List final : public Obj {
public:
	static constexpr size_t DefaultCapacity = 8;
	static constexpr uint GrowthFactor = 2;

	List() : Obj(ObjType::list){};
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

	virtual size_t size() const noexcept override {
		return sizeof(List) + m_capacity * sizeof(Value);
	}

	/// @brief Makes sure there is space for at least 1
	/// more insertion. Ths may grow the list.
	void ensure_capacity();

	const Value& operator[](size_t index) const;
	Value& operator[](size_t index);

private:
	size_t m_capacity = DefaultCapacity;
	size_t m_num_entries = 0;
	Value* m_values = (Value*)malloc(DefaultCapacity * sizeof(Value));

	virtual void trace(GC& gc) noexcept override;
};

} // namespace vyse