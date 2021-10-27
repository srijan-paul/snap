#include <gc.hpp>
#include <vy_list.hpp>

namespace vy {

List::List(size_t mincap)
	: Obj(ObjType::list),
	  m_capacity(pow2ceil(mincap + 1)), m_values{(Value*)malloc(sizeof(Value) * m_capacity)} {
	m_num_entries = mincap;
	for (uint i = 0; i < m_num_entries; ++i) m_values[i] = VYSE_NIL;
}

List::~List() {
	delete[] m_values;
}

void List::ensure_capacity() {
	VYSE_ASSERT(m_capacity >= m_num_entries, "Impossible list capacity.");
	if (m_num_entries + 1 >= m_capacity) {
		m_capacity *= GrowthFactor;
		m_values = (Value*)realloc(m_values, m_capacity * sizeof(Value));
	}
}

void List::append(Value value) {
	ensure_capacity();
	m_values[m_num_entries] = value;
	++m_num_entries;
}

Value List::pop() noexcept {
	if (m_num_entries > 0) {
		return m_values[--m_num_entries];
	}
	return VYSE_NIL;
}

void List::trace(GC& gc) noexcept {
	for (size_t i = 0; i < m_num_entries; ++i) {
		gc.mark_value(m_values[i]);
	}
}

} // namespace vy
