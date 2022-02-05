#pragma once
#include "common.hpp"
#include "forward.hpp"
#include "value.hpp"
#include <cassert>
#include <set>
#include <stack>

namespace vy {

/// @brief A GC lock that protects the object as long as the lock is alive.
/// Once the lock is destroyed, the object is unprotected again. This is useful
/// for protecting an object inside of a certain scope.
/// IMPORTANT: It is advisable to never `gc_unprotect` an object after a lock has been created.
/// The lock will call it automatically upon destruction.
struct GCLock final {
	VYSE_NO_DEFAULT_CONSTRUCT(GCLock);
	VYSE_NO_COPY(GCLock);
	VYSE_NO_MOVE(GCLock);

	GC* const m_gc;
	Obj* const m_object;

	explicit GCLock(GC& gc, Obj* o);
	~GCLock();
};

class GC {
	friend VM;
	friend GCLock;

  public:
	VYSE_NO_DEFAULT_CONSTRUCT(GC);
	VYSE_NO_COPY(GC);
	VYSE_NO_MOVE(GC);

	// first GC is triggered when 1MB is allocated.
	static constexpr size_t InitialGCLimit = 1024 * 1024;

	/// TODO: make this configurable by the user if they demand it.
	static constexpr float GCHeapGrowth = 0.5;

	GC(VM& vm) : m_vm{&vm} {};


	template <typename T>
	void mark(T& value_or_object) {
		if constexpr (std::is_same<T, Value>()) {
			return mark_value(value_or_object);
		} else {
			return mark_object(value_or_object);
		}
	}

	/// @brief If `v` is an object, then marks it as 'alive', preventing
	/// it from being garbage collected.
	void mark_value(Value& v) {
		if (VYSE_IS_OBJECT(v)) mark_object(VYSE_AS_OBJECT(v));
	}

	/// @brief marks an object as 'alive', turning it gray.
	void mark_object(Obj* o);


	private:
	/// @brief Walks over all the entire root set,
	/// marking all objects and coloring them gray.
	void mark();

	/// Marks all the roots reachable from the compiler chain.
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
	/// The VM that calls this GC.
	VM* const m_vm;
	size_t bytes_allocated = 0;
	size_t next_gc = InitialGCLimit;

	/// TODO: Tweak and tune the GC threshholds.

	/// @brief The garbage collector maintains it's personal stack of objects.
	/// During the sweep phase of a GC cycle, this list is traversed and
	/// every object that doesn't have a reference to itself anywhere
	/// else is deleted.
	Obj* m_objects = nullptr;
	std::stack<Obj*> m_gray_objects;

	/// @brief An extra set of GC roots. These are ptrs to
	/// objects marked safe from Garbage Collection.
	std::set<Obj*> m_extra_roots;
};

} // namespace vy
