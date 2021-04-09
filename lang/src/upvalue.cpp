#include <gc.hpp>
#include <upvalue.hpp>

namespace snap {

void Upvalue::trace(GC& gc) {
	gc.mark_value(*m_value);
}

size_t Upvalue::size() const {
	return sizeof(Upvalue);
}

} // namespace snap