#include "value.hpp"
#include <vm.hpp>

namespace vyse {

void GC::mark_value(Value& v) {
	if (VYSE_IS_OBJECT(v)) mark_object(VYSE_AS_OBJECT(v));
}

void GC::mark_object(Obj* o) {
	if (o == nullptr or o->marked) return;
#ifdef VYSE_LOG_GC
	printf("marked: %p [%s] \n", (void*)o, value_to_string(VYSE_OBJECT(o)).c_str());
#endif
	o->marked = true;
	m_gray_objects.push(o);
}

void GC::mark_compiler_roots() {
	Compiler* compiler = m_vm->m_compiler;
	if (compiler == nullptr) return;

	while (compiler != nullptr) {
		mark_object(compiler->m_codeblock);
		compiler = compiler->m_parent;
	}
}

void GC::mark() {

#ifdef VYSE_LOG_GC
	printf("-- [GC start] --\n");
	printf("-- Mark --\n");
#endif

	// The following roots are known atm ->
	// 1. The VM's value stack.
	// 2. Every closure in the call stack.
	// 3. The open upvalue chain.
	// 4. Compiler roots, if the compiler is active.
	// 5. The table of global variables.
	// 6. The 'extra_roots' set.
	// 7. The primitive prototypes in the VM.
	for (Value* v = m_vm->m_stack.values; v < m_vm->m_stack.top; ++v) {
		mark_value(*v);
	}

	for (VM::CallFrame* frame = m_vm->m_current_frame; frame; frame = frame->next) {
		mark_object(frame->func);
	}

	for (Upvalue* uv = m_vm->m_open_upvals; uv != nullptr; uv = uv->next_upval) {
		mark_object(uv);
	}

	for (Obj* o : m_extra_roots) {
		mark_object(o);
	}

	for (auto entry : m_vm->m_global_vars) {
		mark_object(entry.first);
		mark_value(entry.second);
	}

	mark_object(m_vm->prototypes.string);
	mark_object(m_vm->prototypes.number);
	mark_object(m_vm->prototypes.boolean);
	mark_object(m_vm->prototypes.list);

	mark_compiler_roots();
}

void GC::trace() {
#ifdef VYSE_LOG_GC
	printf("-- Trace --\n");
#endif

	while (!m_gray_objects.empty()) {
		Obj* gray_obj = m_gray_objects.top();
		m_gray_objects.pop();

#ifdef VYSE_LOG_GC
		printf("Tracing: %p [%s] \n", (void*)gray_obj, value_to_string(VYSE_OBJECT(gray_obj)).c_str());
#endif
		gray_obj->trace(*this);
	}
}

size_t GC::sweep() {
#ifdef VYSE_LOG_GC
	printf("-- Sweep --\n");
#endif

	/// Delete all the interned strings that haven't been reached
	/// by now.
	m_vm->interned_strings.delete_white_string_keys();

	size_t bytes_freed = 0;

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

#ifdef VYSE_LOG_GC
			printf("Freed: %s", value_to_string(VYSE_OBJECT(current)).c_str());
#endif

			bytes_freed += current->size();
			delete current;
			if (prev == nullptr) {
				m_objects = next;
			} else {
				prev->next = next;
			}
			current = next;
		}
	}

	bytes_allocated -= bytes_freed;
	next_gc = bytes_allocated * (1 + GCHeapGrowth);
#ifdef VYSE_LOG_GC
	printf("-- [GC END] Freed %zu bytes | Next: %zu --\n\n", bytes_freed, next_gc);
#endif
	return bytes_freed;
}

void GC::protect(Obj* o) {
	m_extra_roots.insert(o);
}

void GC::unprotect(Obj* o) {
	m_extra_roots.erase(o);
}

} // namespace vyse
