#pragma once
#include "common.hpp"
#include "forward.hpp"
#include "value.hpp"
#include <set>
#include <stack>

namespace vyse {

class GC {
	friend VM;

public:
	VYSE_NO_DEFAULT_CONSTRUCT(GC);
	VYSE_NO_COPY(GC);
	VYSE_NO_MOVE(GC);

	// first GC is triggered when 1MB is allocated.
	static constexpr size_t InitialGCLimit = 1024 * 1024;

	/// TODO: make this configurable by the user if they demand it.
	static constexpr float GCHeapGrowth = 0.5;

	GC(VM& vm) : m_vm{&vm} {};

	/// @brief Walks over all the entire root set,
	/// marking all objects and coloring them gray.
	void mark();

	/// @brief If `v` is an object, then marks it as 'alive', preventing
	/// it from being garbage collected.
	void mark_value(Value& v);

	/// @brief marks an object as 'alive', turning it gray.
	void mark_object(Obj* o);

	/// Marks all the roots reachable from
	/// the compiler chain.
	void mark_compiler_roots();

	/// @brief Trace all references in the gray stack.
	void trace();

	/// @brief Walks over the list of all known objects,
	/// freeing any object that isn't marked 'alive'.
	/// @return The number of bytes freed.
	size_t sweep();

	/// @brief protects `o` from being garbage collected.
	void protect(Obj* o);
	void unprotect(Obj* o);

private:
	// The VM that calls this GC.
	VM* const m_vm;
	size_t bytes_allocated = 0;
	size_t next_gc = InitialGCLimit;

	/// TODO: Tweak and tune the GC threshholds.

	// The garbage collector maintains it's personal linked
	// list of objects. During the sweep phase of a GC cycle,
	// this list is travesed and every object that doesn't have
	// a reference anywhere else is deleted.
	Obj* m_objects = nullptr;
	std::stack<Obj*> m_gray_objects;

	/// An extra set of GC roots. These are ptrs to
	/// objects marked safe from Garbage Collection.
	std::set<Obj*> m_extra_roots;
};

} // namespace vyse