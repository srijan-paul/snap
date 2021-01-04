#pragma once
#include "block.hpp"
#include "common.hpp"
#include "compiler.hpp"
#include "opcode.hpp"
#include "value.hpp"
#include "vm.hpp"
#include <cstdarg>
#include <functional>
#include <iostream>

namespace snap {
enum class ExitCode { Success, CompileError, RuntimeError };

using VM = class VM;
using PrintFn = std::function<void(const VM& vm, String* string)>;
using ErrorFn = std::function<void(const VM& vm, const char* fstring)>;
using AllocateFn = std::function<void*(VM& vm, size_t old_size, size_t new_size)>;

inline void default_print_fn([[maybe_unused]] const VM& vm, String* string) {
	printf("%s\n", string->chars);
}

void default_error_fn(const VM& vm, const char* message);

// a set of objects maintained by the VM.
// cheap implementation of a vector.
struct ObjectSet {
	static constexpr std::size_t DefaultSize = 16;

	Obj** objects = nullptr;
	std::size_t count = 0;
	std::size_t cap = DefaultSize;

	ObjectSet() : objects(new Obj*[DefaultSize]){};

	void push(Obj* obj) {
		if (count >= cap) {
			cap *= 2;
			objects = (Obj**)realloc(objects, cap);
		}
		objects[count++] = obj;
	}

	Obj* pop() {
		return objects[--count];
	}

	~ObjectSet() {
		free(objects);
	}
};

class VM {
  public:
	VM(const std::string* src);
	~VM();
	Block m_block;
	Compiler m_compiler;

	static constexpr std::size_t StackMaxSize = 256;
	Value m_stack[StackMaxSize];
	Value* sp = m_stack; // points to the next free slot where a value can go

	/// the VM maintains it's personal linked list of objects
	/// for garbage collection.
	Obj* gc_objects = nullptr;
	ObjectSet gray_objects;

	ExitCode interpret();

	/// the function that snap uses to print stuff onto the console.
	/// It is called whenever the `print` function is called in snap source code.
	PrintFn print = default_print_fn;

	/// The function called when there is a compile or runtime error
	/// in the VM. It takes a reference to the VM, and the error
	/// message as a c string.
	ErrorFn log_error = default_error_fn;

	inline Value peek(u8 depth = 0) {
		return *(sp - 1 - depth);
	}

	/// executes the next `count` instructions in the bytecode stream.
	/// by default, count is 1.
	/// @param count number of instructions to excecute.
	inline void step(size_t count = 1) {
		while (count--) run(false);
	}

	/// pushes a value onto the VM's stack.
	/// @param value The Value to push onto the stack.
	inline void push(Value value) {
		*(sp++) = value;
	}

	/// pops a value from the VM stack and returns it.
	inline Value pop() {
		return *(--sp);
	}

	bool init();

	// register an object as 'present' in the object pool.
	// This allows the objectto be marked as available
	// for garbage collection when no longer in use.
	void register_object(Obj* object) {
#ifdef SNAP_STRESS_GC
		collect_garbage();
#endif
		object->next = gc_objects;
		gc_objects = object;
	}

	void collect_garbage();

	ExitCode run(bool run_till_end = true);

  private:
	const std::string* source;

	size_t ip = 0; // instruction ptr

	ExitCode binop_error(const char* opstr, Value& a, Value& b);
	ExitCode runtime_error(const char* fstring...) const;
};

} // namespace snap