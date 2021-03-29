#include "value.hpp"
#include <vm.hpp>

namespace snap {

void GC::mark(Value v) {
	if (SNAP_IS_OBJECT(v)) mark(SNAP_AS_OBJECT(v));
}

void GC::mark(Obj* o) {
	if (o == nullptr or o->marked) return;
#ifdef SNAP_LOG_GC
	printf("marked: %p [%s] \n", (void*)o, value_to_string(SNAP_OBJECT_VAL(o)).c_str());
#endif
	o->marked = true;
	m_gray_objects.push(o);
}

void GC::mark() {

#ifdef SNAP_LOG_GC
	printf("-- GC CYCLE START --\n\n");
	printf("-- Mark --\n");
#endif

	// The following roots are known atm ->
	// 1. The VM's value stack.
	// 2. Every Closure in the call stack.
	// 3. The open upvalue chain.
	// 4. Compiler roots, if the compiler is active.
	// 5. The table of global variables.
	// 6. The 'extra_roots' set.
	for (Value* v = m_vm->m_stack; v < m_vm->sp; ++v) {
		mark(*v);
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
#ifdef SNAP_LOG_GC
	printf("-- Trace --\n");
#endif

	while (!m_gray_objects.empty()) {
		Obj* gray_obj = m_gray_objects.top();
		m_gray_objects.pop();

#ifdef SNAP_LOG_GC
		printf("Tracing: %p [%s] \n", (void*)gray_obj,
			   value_to_string(SNAP_OBJECT_VAL(gray_obj)).c_str());
#endif

		gray_obj->trace(*this);
	}
}

void GC::sweep() {
	// #ifdef SNAP_LOG_GC
	// 	printf("-- Sweep --\n");
	// #endif

	for (Obj* o = m_objects; o != nullptr; o = o->next) {
		if (o->marked) o->marked = false;
	}

#ifdef SNAP_LOG_GC
	printf("-- GC CYCLE END --\n\n");
#endif
}

} // namespace snap