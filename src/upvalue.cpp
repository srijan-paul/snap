#include <gc.hpp>
#include <upvalue.hpp>

namespace vy {

void Upvalue::trace(GC& gc) {
	gc.mark_value(*m_value);
}

} // namespace vy
