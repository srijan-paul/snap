#pragma once
#include "compiler.hpp"
#include "function.hpp"
#include "gc.hpp"
#include "table.hpp"
#include <functional>

namespace snap {

enum class ExitCode : u8 {
	Success,
	CompileError,
	RuntimeError,
};

using PrintFn = std::function<void(const VM& vm, const String* string)>;
using ErrorFn = std::function<void(const VM& vm, std::string& err_message)>;

inline void default_print_fn([[maybe_unused]] const VM& vm, const String* string) {
	SNAP_ASSERT(string != nullptr, "string to print is null.");
	printf("%s", string->c_str());
}

void default_error_fn(const VM& vm, std::string& err_msg);

class VM {
	friend GC;
	friend Compiler;

public:
	SNAP_NO_COPY(VM);
	SNAP_NO_MOVE(VM);

	// The value returned by the VM at the
	// end of it's execution. Nil by default.
	// This stores the value returned by the top level
	// return statement.
	// If this is an object, then that will be
	// destroyed when the VM goes out of scope / reaches
	// the end of it's lifespan.
	Value return_value = SNAP_NIL_VAL;

	/// TODO: Dynamically grow the stack by statically
	/// determining the max stack size at compile time
	/// inside a function body.

	static constexpr size_t StackMaxSize = 256;
	static constexpr size_t MaxCallStack = 128;

	VM(const std::string* src) noexcept : m_source{src}, m_gc(*this){};
	VM() : m_source{nullptr}, m_gc(*this){};
	~VM();
	ExitCode interpret();

	/// the function that snap uses to print stuff onto the console.
	/// It is called whenever the `print` function is called in snap source code.
	PrintFn print = default_print_fn;

	/// The function called when there is a compile or runtime error
	/// in the VM. It takes a reference to the VM, and the error
	/// message as a c string.
	ErrorFn on_error = default_error_fn;

	struct CallFrame {
		/// `func` is either an instance of CClosure
		///  or Closure. However, since it can be
		///  any of the two, we store a pointer to
		///  to it's base class and check which one it
		///  really is at runtime. (using the [tag] field, or
		///  dynamic_cast)
		Obj* func = nullptr;

		size_t ip = 0;
		// the base of the Callframe in the VM's
		// value stack. This denotes the first
		// slot usable by the CallFrame. All local variables
		// are represented as a stack offset from this
		// base.
		Value* base = nullptr;

		bool is_cclosure() const noexcept {
			return func->tag == ObjType::c_closure;
		}
	};

	inline Value peek(u8 depth = 0) const {
		return *(sp - 1 - depth);
	}

	/// @brief Get the [num]th argument of the
	/// current function. get_arg(0) returns the
	/// first argument.
	inline Value& get_arg(u8 idx) const {
		return m_current_frame->base[idx + 1];
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

	/// @brief pop the topmost `n` values from
	/// the stack.
	inline void popn(int n) {
		SNAP_ASSERT(n >= 0, "[n] must be >= 1");
		sp -= n;
	}

	/// @brief Returns the currently exceuting function
	/// wrapped in a value.
	Value current_fn() const {
		return SNAP_OBJECT_VAL(m_current_frame->func);
	}

	Value const* base() const noexcept {
		return m_current_frame->base;
	}

	Value* base() noexcept {
		return m_current_frame->base;
	}

	/// @brief saves the current top
	/// of the stack so it can be restored
	/// later.
	void save_stack() noexcept {
		m_saved_top = sp;
	}

	/// @brief restores the top of the stack
	/// by setting it back to where it was
	/// when [savestack] was last called.
	void restore_stack() noexcept {
		sp = m_saved_top;
	}

	bool init();
	bool call(Value value, u8 argc);
	bool callfunc(Closure* func, int argc);
	bool call_cclosure(CClosure* cclosure, int argc);

	ExitCode runcode(const std::string& code);
	ExitCode runfile(const std::string& filepath);
	ExitCode run();

	const Block* block() const noexcept {
		return m_current_block;
	}

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

	/// TODO: Refactor this logic out from vm.hpp to gc.cpp
	inline void register_object(Obj* o) noexcept {
		SNAP_ASSERT(o != nullptr, "Attempt to register NULL object.");

#ifndef SNAP_STRESS_GC
		if (m_gc.bytes_allocated >= m_gc.next_gc) {
#endif

#ifdef SNAP_LOG_GC
			printf("< GC cycle invoked while attempting to allocate %s >\n",
						 value_to_string(SNAP_OBJECT_VAL(o)).c_str());
#endif
			collect_garbage();

#ifndef SNAP_STRESS_GC
		}
#endif

		o->next = m_gc.m_objects;
		m_gc.m_objects = o;
		m_gc.bytes_allocated += o->size();
	}

	/// TODO: think of a better name for this method.

	/// @brief Makes an interned string and returns a
	/// reference to it.
	String& make_string(const char* chars, size_t length);

	/// @brief Triggers a garbage collection cycle, does a
	/// mark-trace-sweep.
	/// @return The number of bytes freed.
	size_t collect_garbage();

	/// @brief Marks the object safe from garbage collection
	/// until `VM::gc_unprotect` is called on the object.
	void gc_protect(Obj* o) {
		m_gc.protect(o);
	}

	/// @brief If the object was previously marked safe from GC,
	/// then removes the guard, making it garbage collectable again.
	void gc_unprotect(Obj* o) {
		m_gc.unprotect(o);
	}

	/// @brief returns the number of objects objects that haven't been garbage collected.
	size_t num_objects() const;

	/// @brief returns the amount of memory currently allocated by the
	/// VM. Note that this only includes the memory allocated Garbage collectable
	/// objects on the heap and not stack values.
	size_t memory() const noexcept {
		return m_gc.bytes_allocated;
	}

	Value get_global(String* name) const;
	Value get_global(const char* name);

	void set_global(String* name, Value value);
	void set_global(const char* name, Value value);

	/// @brief Loads all the standard library functions into the
	/// VM's global variables table.
	void load_stdlib();

	/// @brief Throws a runtime error by producing a stack trace, then
	/// calling the `on_error` and shutting down the VM by returning an
	/// ExitCode::RuntimeError
	/// @param message The error message.
	ExitCode runtime_error(std::string const& message);

private:
	const std::string* m_source;
	Compiler* m_compiler = nullptr;

	bool m_has_error = false;

	Value m_stack[StackMaxSize];
	using StackId = Value*;

	// The stack pointer pointing to the next free slot
	// in the stack.
	StackId sp = m_stack;

	/// The last saved location of [sp].
	/// This is used to restore the stack's
	/// top with [restore_stack]
	StackId m_saved_top = m_stack;

	// The instruction pointer.
	// It stores the index of the next instruction
	// to be executed. An "instruction" here is an Opcode
	// inside the current function's `m_block`.
	size_t ip = 0;

	/// GARAGE COLLECTION ///

	GC m_gc;

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
	// are being read. This is always `m_current_frame->func->block`
	const Block* m_current_block = nullptr;

	// Snap interns all strings. So if two separate
	// string values are identical, then they point
	// to the same object in heap. To deduplicate
	// strings, we use a table.
	Table interned_strings;

	std::unordered_map<String*, Value> m_global_vars;

	/// @brief concatenates two strings. Might intern the resulting
	/// string if it isn't already interned.
	Value concatenate(const String* left, const String* right);

	/// Load a snap::Object into the global variable list.
	/// generally used for loading functions and objects from
	/// the standard library.
	void add_stdlib_object(const char* name, Obj* o);

	/// @brief prepares for a functionc call by pushing a new
	/// CallFrame onto the call stack. The new frame's func field
	/// is set to [callable], and the ip of the current call frame
	/// is cached.
	void push_callframe(Obj* callable, int argc);

	/// @brief Pops the currently active CallFrame off of the
	/// call stack, and restores the state of the previous
	/// CallFrame.
	void pop_callframe();

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
};

} // namespace snap