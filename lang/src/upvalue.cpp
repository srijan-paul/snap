#include <gc.hpp>
#include <upvalue.hpp>

namespace vyse {

void Upvalue::trace(GC& gc) {
	gc.mark_value(*m_value);
}

} // namespace vyse 