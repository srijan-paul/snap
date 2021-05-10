#include <vy_list.hpp>
#include <gc.hpp>

namespace vyse {

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

Value& List::operator[](size_t index) {
  VYSE_ASSERT(index < m_num_entries, "List index out of range!");
  return m_values[index];
}

const Value& List::operator[](size_t index) const {
  VYSE_ASSERT(index < m_num_entries, "List index out of range!");
  return m_values[index];
}

void List::trace(GC& gc) noexcept {
  for (size_t i = 0; i < m_num_entries; ++i) {
    gc.mark_value(m_values[i]);
  }
}

} // namespace vyse