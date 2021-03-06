#pragma once
#include "compiler.hpp"
#include "gc.hpp"
#include "table.hpp"
#include "vm_stack.hpp"
#include <functional>

namespace vyse {

enum class ExitCode : u8 {
	Success,
	CompileError,
	RuntimeError,
};

using PrintFn = std::function<void(const VM& vm, const String* string)>;
using ReadLineFn = std::function<char*(const VM& vm)>;
using ErrorFn = std::function<void(const VM& vm, std::string& err_message)>;

inline void default_print_fn([[maybe_unused]] const VM& vm, const String* string) {
	VYSE_ASSERT(string != nullptr, "string to print is null.");
	printf("%s", string->c_str());
}

void default_error_fn(const VM& vm, std::string& err_msg);
char* default_readline(const VM& vm);

class VM {
	friend GC;
	friend Compiler;

public:
	VYSE_NO_COPY(VM);
	VYSE_NO_MOVE(VM);

	// The value returned by the VM at the
	// end of it's execution. Nil by default.
	// This stores the value returned by the top level
	// return statement.
	// If this is an object, then that will be
	// destroyed when the VM goes out of scope / reaches
	// the end of it's lifespan.
	Value return_value = VYSE_NIL;

	/// TODO: Dynamically grow the stack by statically
	/// determining the max stack size at compile time
	/// inside a function body.

	/// the function that vyse uses to print stuff onto the console.
	/// It is called whenever the `print` function is called in vyse source code.
	PrintFn print = default_print_fn;

	/// The function called when there is a compile or runtime error
	/// in the VM. It takes a reference to the VM, and the error
	/// message as a c string.
	ErrorFn on_error = default_error_fn;

	/// The function to be used by the VM when reading a line from stdin.
	ReadLineFn read_line = default_readline;

	/// Maximum size of the call stack. If the call stack
	/// size exceeds this, then there is a stack overflow.
	static constexpr size_t MaxCallStack = 1024;

	/// Maximum depth of the stack trace show in error messages.
	/// We put a cap on this to avoid extrememly long stack traces
	/// caused by infinite recursion.
	static constexpr size_t MaxStackTraceDepth = 11;

	/// @brief The VM's value stack. All operations in Vyse
	/// are done by popping from and pushing to this data
	/// structure.
	Stack m_stack;

	VM(const std::string* src) noexcept : m_source{src}, m_gc(*this){};
	VM() : m_source{nullptr}, m_gc(*this){};
	~VM();
	ExitCode interpret();

	struct CallFrame {
		/// `func` is either an instance of CClosure
		///  or Closure. However, since it can be
		///  any of the two, we store a pointer to
		///  to it's base class and check which one it
		///  really is at runtime. (using the [tag] field)
		Obj* func = nullptr;

		/// The next instruction to be executed
		/// In this function.
		size_t ip = 0;

		/// the base of the Callframe in the VM's
		/// value stack. This denotes the first
		/// slot usable by the CallFrame. All local variables
		/// are represented as a stack offset from this
		/// base.
		Value* base = nullptr;

		CallFrame* next = nullptr;
		CallFrame* prev = nullptr;

		bool is_cclosure() const noexcept {
			return func->tag == ObjType::c_closure;
		}
	};

	/// @brief Get the [num]th argument of the
	/// current function. get_arg(0) returns the
	/// first argument.
	inline Value& get_arg(u8 idx) const {
		return m_current_frame->base[idx + 1];
	}

	/// @brief Returns the currently exceuting function
	/// wrapped in a value.
	Value current_fn() const {
		return VYSE_OBJECT(m_current_frame->func);
	}

	Value const* base() const noexcept {
		return m_current_frame->base;
	}

	Value* base() noexcept {
		return m_current_frame->base;
	}

	bool init();

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
		VYSE_ASSERT(o != nullptr, "Attempt to register NULL object.");

#ifndef VYSE_STRESS_GC
		if (m_gc.bytes_allocated >= m_gc.next_gc) {
#endif

#ifdef VYSE_LOG_GC
			printf("< GC cycle invoked while attempting to allocate %s >\n",
						 value_to_string(VYSE_OBJECT(o)).c_str());
#endif
			collect_garbage();

#ifndef VYSE_STRESS_GC
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

	/// @brief Makes a string from the provided char buffer.
	/// @param chars A null terminated char buffer.
	String& make_string(const char* chars) {
		return make_string(chars, strlen(chars));
	}

	/// @brief takes ownership of a string with char buffer 'chrs'
	/// and length 'len'. Note that `chrs` now belongs to the VM,
	/// and it may be freed inside this function if an interned copy
	/// is found. The caller must not use the [chrs] buffer after
	/// calling this.
	String& take_string(char* chrs, size_t len);

	/// @brief Triggers a garbage collection cycle, does a
	/// mark-trace-sweep.
	/// @return The number of bytes freed.
	size_t collect_garbage();

	/// @brief Makes sure there are at least [num_slots] stack slots
	/// free to be used above the current stack-top.
	void ensure_slots(uint num_slots);

	/// @brief turns off the garbage collector.
	/// GC cycles won't be triggered regardless of
	/// how much memory is allocated.
	inline void gc_off() {
		can_collect = false;
	}

	/// @brief turns on the garbage collector.
	inline void gc_on() {
		can_collect = true;
	}

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

	/// @brief calls a callable object that is present at a depth
	/// of [argc] - 1 in the stack, followed by argc arguments.
	/// @param argc number of a arguments.
	/// @return true if the function call was sucessfull, false if it
	///	resulted in an error.
	bool call(int argc);

	Value get_global(String* name) const;
	Value get_global(const char* name);

	void set_global(String* name, Value value);
	void set_global(const char* name, Value value);

	/// @brief Loads all the standard library functions into the
	/// VM's global variables table.
	void load_stdlib();

	/// @brief loads the prototypes of all
	/// primitive data types.
	void load_primitives();

	/// @brief Throws a runtime error by producing a stack trace, then
	/// calling the `on_error` and shutting down the VM by returning an
	/// ExitCode::RuntimeError
	/// @param message The error message.
	ExitCode runtime_error(std::string const& message);

	// Prototypes for primitive data types.
	struct PrimitiveProtos {
		Table* string = nullptr;
		Table* number = nullptr;
		Table* boolean = nullptr;
		Table* list = nullptr;
	} prototypes;

private:
	const std::string* m_source;
	bool m_has_error = false;
	Compiler* m_compiler = nullptr;

	/// @brief Whether or not the garbage collector
	/// is allowed to collect garbage. This can be turned
	/// on or off using `VM::gc_off`
	bool can_collect = true;

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

	/// the call stack is internally a linked list,
	/// the base call frame is always fixed and initalized
	/// when the VM is constructed.
	CallFrame* const base_frame = new CallFrame;

	/// The topmost callframe that the VM is
	/// currently executing in.
	CallFrame* m_current_frame = base_frame;
	// total number of call frames that have been allocated.
	u32 m_frame_count = 0;

	// current block from which the opcodes
	// are being read. This is always `m_current_frame->func->block`
	const Block* m_current_block = nullptr;

	// Vyse interns all strings. So if two separate
	// string values are identical, then they point
	// to the same object in heap. To deduplicate
	// strings, we use a table.
	Table interned_strings;

	std::unordered_map<String*, Value> m_global_vars;

	/// @brief call any callable value from within the VM.
	/// Note that this is only used to call instructions from
	/// inside a vyse script. To call anything from a C/C++
	/// program, the call method is used instead.
	bool op_call(Value value, u8 argc);
	bool call_closure(Closure* func, int argc);
	bool call_cclosure(CClosure* cclosure, int argc);

	/// @brief return the `table[key]` where table is the prototype
	/// of [value].
	inline Value index_value(const Value& value, const Value& key) noexcept {
		switch (VYSE_GET_TT(value)) {
		case ValueType::Bool: return prototypes.boolean->get(key);
		case ValueType::Number: return prototypes.number->get(key);
		case ValueType::Object: {
			const Obj* o = VYSE_AS_OBJECT(value);
			switch (o->tag) {
			case ObjType::string: return prototypes.string->get(key);
			case ObjType::list: return prototypes.list->get(key);
			case ObjType::table: return static_cast<const Table*>(o)->get(key);
			default: return VYSE_NIL;
			}
		}
		default: return VYSE_NIL;
		}
	}

	/// @brief concatenates two strings. Will intern the resulting
	/// string if it isn't already interned.
	Value concatenate(const String* left, const String* right);

	/// @return return the [index]th character in a string.
	/// String characters are 0 indexed. So 'abc'[0] is a.
	String* char_at(const String* string, uint index);

	/// Load a vyse::Object into the global variable list.
	/// generally used for loading functions and objects from
	/// the standard library.
	void add_stdlib_object(const char* name, Obj* o);

	/// @brief Prepares the VM CallStack for the very first
	/// function call, which is the toplevel userscript.
	void invoke_script(Closure* closure);

	/// @brief prepares for a function call by pushing a new
	/// CallFrame onto the call stack. The new frame's func field
	/// is set to [callable], and the ip of the current call frame
	/// is cached.
	void push_callframe(Obj* callable, int argc);

	/// @brief Pops the currently active CallFrame off of the
	/// call stack, and restores the state of the previous
	/// CallFrame.
	void pop_callframe() noexcept;

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
	ExitCode binop_error(const char* opstr, const Value& a, const Value& b);
};

} // namespace vyse