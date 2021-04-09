#include <gc.hpp>
#include <upvalue.hpp>

namespace snap {

void Upvalue::trace(GC& gc) {
	gc.mark_value(*m_value);
}

} // namespace snap