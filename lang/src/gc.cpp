#include "gc.hpp"
#include <compiler.hpp>
#include <value.hpp>

namespace snap {

using OT = ObjType;

static void mark_value(VM& vm, Value& value);
static void mark_object(VM& vm, Obj* object);
static void mark_compiler(VM& vm);

void GC::mark_roots(VM& vm) {
	for (Value* value = vm.m_stack; value < vm.sp; value++) {
		mark_value(vm, *value);
	}
	mark_compiler(vm);
}

static void mark_block(VM& vm, Block* block) {
	for (Value& v : block->constant_pool) {
		mark_value(vm, v);
	}
}

static void mark_compiler(VM& vm) {
	Compiler& compiler = vm.m_compiler;
	mark_block(vm, compiler.m_current_block);
}

static void mark_value(VM& vm, Value& value) {
	if (SNAP_IS_OBJECT(value)) {
		mark_object(vm, SNAP_AS_OBJECT(value));
	}
}

static void mark_object(VM& vm, Obj* object) {
	if (object == nullptr || object->marked) return;

#ifdef SNAP_LOG_GC
	printf("mark: %p [", (void*)object);
	print_value(SNAP_OBJECT_VAL(object));
	printf("].\n");
#endif

	object->marked = true;
	vm.gray_objects.push(object);
}

static void blacken_object(Obj* obj) {
	switch (obj->tag) {
	case OT::string: break;
	}
}

void GC::trace_refs(VM& vm) {
	while (vm.gray_objects.count > 0) {
		blacken_object(vm.gray_objects.pop());
	}
}

void GC::free_object(Obj* object) {
#ifdef SNAP_LOG_GC
	printf("freed: %p	", (void*)object);
#endif
	switch (object->tag) {
	case OT::string: {
#ifdef SNAP_LOG_GC
		printf("[string: '%s']\n", static_cast<String*>(object)->chars);
#endif
		static_cast<String*>(object)->~String();
		break;
	default: delete object; break;
	}
	}
}

void GC::sweep(VM& vm) {
	Obj* prev = nullptr;
	Obj* object = vm.gc_objects;

	while (object != nullptr) {
		if (object->marked) {
			object->marked = false;
			prev = object;
			object = object->next;
		} else {
			Obj* unreachable = object;
			object = object->next;

			if (prev != nullptr) {
				prev->next = object;
			} else { // if the very first object was unreachable.
				vm.gc_objects = object;
			}
			free_object(unreachable);
		}
	}
	printf("\n");
}

} // namespace snap