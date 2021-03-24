#pragma once
#include "compiler.hpp"
#include "table.hpp"
#include "value.hpp"
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <functional>
#include <iostream>
#include <set>

namespace snap {
enum class ExitCode : u8 { Success, CompileError, RuntimeError };

using PrintFn = std::function<void(const VM& vm, String* string)>;
using ErrorFn = std::function<void(const VM& vm, std::string& err_message)>;

inline void default_print_fn([[maybe_unused]] const VM& vm, String* string) {
	printf("%s\n", string->c_str());
}

void default_error_fn(const VM& vm, std::string& err_msg);

struct CallFrame {
	Function* func;
	size_t ip;
	// the base of the Callframe in the VM's
	// value stack. This denotes the first
	// slot usable by the CallFrame. All local variables
	// are represented as a stack offset from this
	// base.
	Value* base;
};

class GC {
  public:
	// the VM maintains it's personal linked list of objects
	// for garbage collection. GC is achieved by walking over
	// this list, and removing all objects that have no other
	// references anywhere else in the VM
	Obj* m_objects = nullptr;
	std::vector<Obj*> m_gray_objects;

	// An extra set of GC roots. These are ptrs to
	// objects marked safe from Garbage Collection.
	std::set<Obj*> m_extra_roots;

	/// @brief If `v` is an object, then marks it as 'alive', preventing
	/// it from being garbage collected.
	void mark(Value v);
	/// @brief marks an object as 'alive', turning it gray.
	void mark(Obj* o);

	/// @brief trace all references in the gray stack.
	void trace();

	/// @brief protects `o` from being garbage collected.
	void protect(Obj* o);
	void unprotect(Obj* o);
};

class VM {
	friend class GC;

  public:
	VM(const std::string* src);
	~VM();

	Compiler* m_compiler;

	// The value returned by the VM at the
	// end of it's execution. Nil by default.
	// This stores the value returned by the top level
	// return statement.
	// If this is an object, then that will be
	// destroyed when the VM goes out of scope / reaches
	// the end of it's lifespan.
	Value return_value = SNAP_NIL_VAL;

	static constexpr size_t StackMaxSize = 256;
	static constexpr size_t MaxCallStack = 128;
	Value m_stack[StackMaxSize];
	// points to the next free slot where a value can go

	using StackId = Value*;
	StackId sp	  = m_stack;

	ExitCode interpret();

	/// the function that snap uses to print stuff onto the console.
	/// It is called whenever the `print` function is called in snap source code.
	PrintFn print = default_print_fn;

	/// The function called when there is a compile or runtime error
	/// in the VM. It takes a reference to the VM, and the error
	/// message as a c string.
	ErrorFn on_error = default_error_fn;

	inline Value peek(u8 depth = 0) const {
		return *(sp - 1 - depth);
	}

	/// pushes a value onto the VM's stack.
	/// @param value The Value to push onto the stack.
	inline void push(Value value) {
		*(sp++) = value;
	}

	/// @brief pops a value from the VM stack and returns it.
	inline Value pop() {
		return *(--sp);
	}

	bool init();
	bool call(Value value, u8 argc);
	bool callfunc(Function* func, int argc);

	ExitCode run();
	const Block* block();

	/// @brief makes an object of type [T],
	/// registers it with the VM and return a reference to
	/// the newly created object.
	/// The object is registered simply by attaching
	/// it to the head of the VM's object linked list.
	template <typename T, typename... Args>
	T& make(Args&&... args) {
		T* object = new T(std::forward<Args>(args)...);
		register_object(object);
		return *object;
	}

	inline void register_object(Obj* o) {
		assert(o != nullptr);
		o->next		   = m_gc.m_objects;
		m_gc.m_objects = o;
	}

	/// @brief Makes an interned string and returns a
	/// reference to it.
	String& string(const char* chars, size_t length);

	/// @brief Triggers a garbage collection cycle, does a
	/// mark-trace-sweep.
	void collect_garbage();

	/// @brief Marks the object safe from garbage collection
	/// until `VM::gc_unprotect` is called on the object.
	void gc_protect(Obj* o);

	/// @brief If the object was previously marked safe from GC,
	/// then removes the guard, making it garbage collectable again.
	void gc_unprotect(Obj* o);

	/// @brief returns the number of objects present in `gc.m_objects.`
	size_t num_objects();

  private:
	const std::string* m_source;

	// The instruction pointer.
	// It stores the index of the next instruction
	// to be executed. An "instruction" here is an Opcode
	// inside the current function's `m_block`.
	size_t ip = 0;

	/// GARAGE COLLECTION ///

	GC m_gc;

	/// @brief triggers the 'mark' phase of garbage collection.
	/// All known roots are marked as 'alive' and turned gray.
	void mark();

	/// @brief traces all the 'gray' (untraced) objects in the object
	/// graph, traversing through their references and marking any previously
	/// unreached object as 'alive'.
	void trace();

	/// @brief Triggers the 'sweep' phase, traversing the entire
	/// object graph and freeing any object that isn't marked alive.
	void sweep();

	// VM's personal list of all open upvalues.
	// This is a sorted linked list, the head
	// contains the upvalue pointing to the
	// highest value on the stack.
	Upvalue* m_open_upvals = nullptr;

	CallFrame m_frames[MaxCallStack];
	CallFrame* m_current_frame = m_frames;
	// total number of call frames that have been allocated.
	u32 m_frame_count = 0;

	// current block from which the opcodes
	// are being read. This always `m_current_frame->func->block`
	const Block* m_current_block = nullptr;

	// Snap interns all strings. So if two separate
	// string values are identical, then they point
	// to the same object in heap. To deduplicate
	// strings, we use a table.
	Table interned_strings;

	/// Wrap a value present at stack slot [slot]
	/// inside an Upvalue object and add it to the
	/// VM's currently open Upvalue list in the right
	/// position (if it isn't already there).
	/// @param slot A `Value*` pointing to a value inside the VM's
	///             value stack.
	Upvalue* capture_upvalue(Value* slot);
	// close all the upvalues that are present between the
	// top of the stack and [last].
	// [last] must point to some value in the VM's stack.
	void close_upvalues_upto(Value* last);

	/// @brief Throw an error caused by a binary operator's bad operand types.
	/// @param opstr a C string representing the binary operator. (eg - "+")
	/// @param a The left operand
	/// @param b The right operand
	ExitCode binop_error(const char* opstr, Value& a, Value& b);

	/// @brief Throws a runtime error by producing a stack trace, then
	/// calling the `on_error` and shutting down the VM by returning an
	/// ExitCode::RuntimeError
	/// @param message The error message.
	ExitCode runtime_error(std::string&& message);
};

} // namespace snap