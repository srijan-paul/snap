#pragma once
#include "block.hpp"
#include "common.hpp"
#include "opcode.hpp"
#include <cstdarg>
#include <functional>
#include <iostream>

namespace snap {
enum class ExitCode { Success, CompileError, RuntimeError };

using VM = class VM;
using PrintFn = std::function<void(const VM& vm, String* string)>;
using ErrorFn = std::function<void(const VM& vm, const char* fstring)>;
using AllocateFn = std::function<void*(VM& vm, size_t old_size, size_t new_size)>;

inline void default_print_fn(const VM& vm, String* string) {
	printf("%s\n", string->chars);
}

void default_error_fn(const VM& vm, const char* message);

class VM {
  public:
	VM(const std::string* src);
	Block m_block;
	ExitCode interpret();

	/// the function that snap uses to print stuff onto the console.
	/// It is called whenever the `print` function is called in snap source code.
	PrintFn print = default_print_fn;

	/// The function called when there is a compile or runtime error
	/// in the VM. It takes a reference to the VM, and the error
	/// message as a c string.
	ErrorFn log_error = default_error_fn;

	/// The memory allocator used by snap's garbage collector.
	/// This function is called whenever any memory is moved, freed or requested
	/// on the heap.
	AllocateFn allocator;

	inline Value peek(u8 depth = 0) {
		return *(sp - 1 - depth);
	}

	static constexpr size_t StackMaxSize = 256;

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
	ExitCode run(bool run_till_end = true);

  private:
	const std::string* source;
	/// the VM maintains it's personal linked list of objects
	/// for garbage collection.
	Obj* gc_objects;

	size_t ip = 0; // instruction ptr
	
	Value m_stack[StackMaxSize];
	Value* sp = m_stack;

	ExitCode binop_error(const char* opstr, Value& a, Value& b);
	ExitCode runtime_error(const char* fstring...) const;
};

} // namespace snap