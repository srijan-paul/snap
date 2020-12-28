#pragma once
#include "block.hpp"
#include "opcode.hpp"
#include "ast.hpp"
#include <functional>
#include <iostream>

namespace snap {
enum class ExitCode { Success, CompileError, RuntimeError };

using VM = class VM;
using PrintFn = std::function<void(const VM& vm, const char* string)>;
using ErrorFn = std::function<void(const VM& vm, const char* err_message)>;
using AllocateFn = std::function<void*(VM& vm, size_t old_size, size_t new_size)>;

inline void defaultPrintFn(const VM& vm, const char* string) {
	printf("%s", string);
}

class VM {
  public:
	VM(const std::string* src);
	ExitCode interpret();
	/// the function that snap uses to print stuff onto the console.
	/// It is called whenever the `print` function is called in snap source code.
	PrintFn print = defaultPrintFn;

	/// The memory allocator used by snap's garbage collector.
	/// This function is called whenever any memory is moved, freed or requested
	/// on the heap.
	AllocateFn allocator;

	inline Value peek(u8 depth = 0) {
		return m_stack[sp - 1 - depth];
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
		m_stack[sp++] = value;
	}

	/// pops a value from the VM stack and returns it.
	inline Value pop() {
		return m_stack[sp--];
	}

	bool init();
	ExitCode run(bool run_till_end = true);

  private:
	const std::string* source;
	/// the VM maintains it's personal linked list of objects
	/// for garbage collection.
	Obj* gc_objects;
	Block m_block;
	size_t ip = 0; // instruction ptr
	size_t sp = 0; // stack-top ptr
	Value m_stack[StackMaxSize];
};

} // namespace snap