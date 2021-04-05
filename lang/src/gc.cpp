#include "value.hpp"
#include <vm.hpp>

namespace snap {

void GC::mark_value(Value v) {
	if (SNAP_IS_OBJECT(v)) mark_object(SNAP_AS_OBJECT(v));
}

void GC::mark_object(Obj* o) {
	if (o == nullptr or o->marked) return;
#ifdef SNAP_LOG_GC
	printf("marked: %p [%s] \n", (void*)o, value_to_string(SNAP_OBJECT_VAL(o)).c_str());
#endif
	o->marked = true;
	m_gray_objects.push(o);
}

void GC::mark_compiler_roots() {
	Compiler* compiler = m_vm->m_compiler;
	if (compiler == nullptr) return;

	while (compiler != nullptr) {
		mark_object(compiler->m_proto);
		compiler = compiler->m_parent;
	}
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
		mark_value(*v);
	}

	for (CallFrame* frame = m_vm->m_frames; frame < m_vm->m_current_frame; ++frame) {
		mark_object(frame->func);
	}

	for (Upvalue* uv = m_vm->m_open_upvals; uv != nullptr; uv = uv->next_upval) {
		mark_object(uv);
	}

	for (Obj* o : m_extra_roots) {
		mark_object(o);
	}

	mark_compiler_roots();
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
#ifdef SNAP_LOG_GC
	printf("-- Sweep --\n");
#endif

	/// Delete all the interned strings that haven't been reached
	/// by now.
	m_vm->interned_strings.delete_white_string_keys(*this);

	// Walk over the [m_objects] linked list
	// and free any objects that aren't marked
	// alive.
	Obj* prev = nullptr;
	Obj* current = m_objects;
	while (current != nullptr) {
		if (current->marked) {
			current->marked = false;
			prev = current;
			current = current->next;
		} else {
			Obj* next = current->next;

#ifdef SNAP_LOG_GC
			printf("Freed: %s\n", value_to_string(SNAP_OBJECT_VAL(current)).c_str());
#endif

			delete current;
			if (prev == nullptr) {
				m_objects = next;
			} else {
				prev->next = next;
			}
			current = next;
		}
	}

#ifdef SNAP_LOG_GC
	printf("-- GC CYCLE END --\n\n");
#endif
}

void GC::protect(Obj* o) {
	m_extra_roots.insert(o);
}

void GC::unprotect(Obj* o) {
	SNAP_ASSERT(m_extra_roots.find(o) != m_extra_roots.end(),
				"Attempt to unprotect an object which hasn't been protected.");
	m_extra_roots.erase(o);
}

} // namespace snap