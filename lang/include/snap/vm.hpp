#pragma once
#include "compiler.hpp"
#include <cstdarg>
#include <cstddef>
#include <functional>
#include <iostream>

namespace snap {
enum class ExitCode : u8 { Success, CompileError, RuntimeError };

using PrintFn = std::function<void(const VM& vm, String* string)>;
using ErrorFn = std::function<void(const VM& vm, const char* fstring)>;
using AllocateFn = std::function<void*(VM& vm, size_t old_size, size_t new_size)>;

inline void default_print_fn([[maybe_unused]] const VM& vm, String* string) {
	printf("%s\n", string->c_str());
}

void default_error_fn(const VM& vm, const char* message);

struct CallFrame {
	Function* func;
	std::size_t ip;
	// the base of the Callframe in the VM's
	// value stack. This denotes the first
	// slot usable by the CallFrame. All local variables
	// are represented as a stack offset from this
	// base.
	StackId base;
};

class VM {
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

	static constexpr std::size_t StackMaxSize = 256;
	static constexpr std::size_t MaxCallStack = 128;
	Value m_stack[StackMaxSize];
	// points to the next free slot where a value can go
	StackId sp = m_stack;

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
	/// @param count number of instructions to excecute. 1 by default.
	inline void step(size_t count = 1) {
		while (count--) run(false);
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

	void collect_garbage();
	ExitCode run(bool run_till_end = true);
	const Block* block();

  /// @brief makes an object of type [T], 
	/// registers it with the VM and return a reference to
	/// the newly created object.
	/// The object is registered simply by attaching 
	/// it to the head of the VM's object linked list.
	template<typename T, typename... Args>
	T& make(Args&&... args) {
		T* object = new T(std::forward<Args>(args)...);
		object->next = m_gc_objects;
		m_gc_objects = object;
		return *object;
	}

  private:
	const std::string* m_source;

	// The instruction pointer.
	// It stores the index of the next instruction
	// to be executed. An "instruction" here is an Opcode
	// inside the current function's `m_block`.
	std::size_t ip = 0;

	// the VM maintains it's personal linked list of objects
	// for garbage collection. GC is achieved by walking over 
	// this list, and removing all objects that have no other
	// references anywhere else in the VM
	Obj* m_gc_objects = nullptr;
	std::vector<Obj*> m_gray_objects;

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
	Block* m_current_block = nullptr;

	/// Wrap a value present at stack slot [slot]
	/// inside an Upvalue object and add it to the
	/// VM's currently open Upvalue list in the right
	/// position (if it isn't already there).
	/// @param slot A `Value*` referencing a valueinside the VM's 
	///             value stack.
	Upvalue* capture_upvalue(Value* slot);
	// close all the upvalues that are present between the
	// top of the stack and [last].
	// [last] must point to some value in the VM's stack.
	void close_upvalues_upto(Value* last);

	/// @brief Throw an error caused by a binary operator's bad operand types.
	/// @param opstr a `C string representing the binary operator. (eg - "+")
	/// @param a The left operand
	/// @param b The right operand
	ExitCode binop_error(const char* opstr, Value& a, Value& b);

	/// @brief Throws a runtime error by calling the `log_error` and
	/// then shutting down the VM by returning an ExitCode::RuntimeError
	/// @param fstring the format string, followed by format args. 
	///        Similar to `printf`.
	ExitCode runtime_error(const char* fstring...) const;
};

} // namespace snap