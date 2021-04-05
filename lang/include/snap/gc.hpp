#pragma once
#include "common.hpp"
#include "forward.hpp"
#include "value.hpp"
#include <set>
#include <stack>

namespace snap {

class GC {
	friend VM;

  public:
	SNAP_NO_DEFAULT_CONSTRUCT(GC);
	SNAP_NO_COPY(GC);
	SNAP_NO_MOVE(GC);

	GC(VM& vm) : m_vm{&vm} {};

	/// @brief Walks over all the entire root set,
	/// marking all objects and coloring them gray.
	void mark();

	/// @brief If `v` is an object, then marks it as 'alive', preventing
	/// it from being garbage collected.
	void mark_value(Value v);

	/// @brief marks an object as 'alive', turning it gray.
	void mark_object(Obj* o);

	/// Marks all the roots reachable from
	/// the compiler chain.
	void mark_compiler_roots();

	/// @brief Trace all references in the gray stack.
	void trace();

	/// @brief Walks over the list of all known objects,
	/// freeing any object that isn't marked 'alive'.
	void sweep();

	/// @brief protects `o` from being garbage collected.
	void protect(Obj* o);
	void unprotect(Obj* o);

  private:
	// The VM that calls this GC.
	VM* const m_vm;

	// the Garbage collector maintains it's personal linked
	// list of objects this list, and removing all objects
	// that have no other references anywhere else in the VM.
	Obj* m_objects = nullptr;
	std::stack<Obj*> m_gray_objects;

	/// An extra set of GC roots. These are ptrs to
	/// objects marked safe from Garbage Collection.
	std::set<Obj*> m_extra_roots;
};

} // namespace snap