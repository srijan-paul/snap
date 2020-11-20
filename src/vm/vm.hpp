#pragma once
#include "../block.hpp"
#include "../opcode.hpp"
#include "../syntax/ast/ast.hpp"
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
	PrintFn print = &defaultPrintFn;

	/// The memory allocator used by snap's garbage collector.
	/// This function is called whenever any memory is moved, freed or requested
	/// on the heap.
	AllocateFn allocator;

	inline Value peek(u8 depth) {
		return m_stack[sp - 1 - depth];
	}
	static constexpr size_t StackMaxSize = 256;

	/// executes the next `count` instructions in the bytecode stream.
	/// by default, count is 1.
	inline void step(size_t count = 1) {
		while (count--) run(false);
	}

	inline void push(Value value) {
		m_stack[sp++] = value;
	}

	inline Value pop() {
		return m_stack[sp--];
	}

	bool init();

  private:
	const std::string* source;
	Block m_block;
	size_t ip = 0; // instruction ptr
	size_t sp = 0; // stack-top ptr
	ExitCode run(bool run_till_end = true);
	Value m_stack[StackMaxSize];
};

} // namespace snap