#include <upvalue.hpp>
#include <gc.hpp>

namespace snap {

void Upvalue::trace(GC& gc) {
  gc.mark_value(*m_value);
}

} // namespace snap