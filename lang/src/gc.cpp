#include "value.hpp"
#include <vm.hpp>

namespace snap {

void GC::mark(Value v) {
	if (SNAP_IS_OBJECT(v)) mark(SNAP_AS_OBJECT(v));
}

void GC::mark(Obj* o) {
	if (o == nullptr or o->marked) return;
	o->marked = true;
  m_gray_objects.push_back(o);
}

void GC::mark() {
	// The following roots are known atm ->
	// 1. The VM's value stack.
	// 2. Every Closure in the call stack.
	// 3. The open upvalue chain.
	// 4. Compiler roots, if the compiler is active.
	// 5. The table of global variables.
	// 6. The 'extra_roots' set.
	for (Value* v = m_vm->m_stack; v < m_vm->sp; ++v) {
		mark(v);
	}

	for (CallFrame* frame = m_vm->m_frames; frame < m_vm->m_current_frame; ++frame) {
		mark(frame->func);
	}

	for (Upvalue* uv = m_vm->m_open_upvals; uv != nullptr; uv = uv->next_upval) {
		mark(uv);
	}

	for (Obj* o : m_extra_roots) {
		mark(o);
	}
}

void GC::trace() {
}

void GC::sweep() {
}

} // namespace snap