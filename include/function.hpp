#pragma once
#include "string.hpp"
#include "upvalue.hpp"

namespace vy {

// A protoype is the body of a function that contains the bytecode and other relevant information.
class CodeBlock final : public Obj {
	friend Compiler;

  public:
	explicit CodeBlock(String* funcname) noexcept : Obj{ObjType::codeblock}, m_name{funcname} {};
	explicit CodeBlock(String* funcname, u32 param_count) noexcept
		: Obj{ObjType::codeblock}, m_name{funcname}, m_num_params{param_count} {};

	~CodeBlock(){};

	[[nodiscard]] constexpr const String* name() const noexcept {
		return m_name;
	}

	[[nodiscard]] constexpr const char* name_cstr() const noexcept {
		return m_name->c_str();
	}

	[[nodiscard]] constexpr Block& block() noexcept {
		return m_block;
	}

	[[nodiscard]] constexpr const Block& block() const noexcept {
		return m_block;
	}

	u32 add_param();

	[[nodiscard]] constexpr u32 param_count() const noexcept {
		return m_num_params;
	}

	[[nodiscard]] size_t size() const override {
		return sizeof(CodeBlock);
	}

	[[nodiscard]] constexpr size_t stack_size() const noexcept {
		return max_stack_size;
	}

	[[nodiscard]] constexpr bool is_vararg() const noexcept {
		return m_is_variadic;
	}

  private:
	String* const m_name;
	u32 m_num_params = 0;
	u32 m_num_upvals = 0;
	/// @brief the maximum stack size ever needed for the execution of this function's call frame.
	/// This is computed by the compiler.
	int max_stack_size = 0;
	Block m_block;

	/// @brief Whether this function accepts a varying number of arguments.
	bool m_is_variadic = false;

	void trace(GC& gc) override;
};

/// @brief A closure has two parts, code and data. The code part is represented by the prototype
/// containing all the bytecode instructions and the data part is represented by the upvalues vector
/// holding all the captured variables from enclosing scopes.
class Closure final : public Obj {
  public:
	CodeBlock* const m_codeblock;

	explicit Closure(CodeBlock* proto, u32 upval_count) noexcept;
	~Closure() override{};

	[[nodiscard]] constexpr const String* name() const noexcept {
		return m_codeblock->name();
	}

	[[nodiscard]] constexpr const char* name_cstr() const noexcept {
		return m_codeblock->name_cstr();
	}

	/// @brief returns the Upvalue at index [idx] in the
	/// upvalue list.
	[[nodiscard]] Upvalue* get_upval(u32 idx) noexcept {
		VYSE_ASSERT(idx < m_upvals.size(), "Invalid upvalue index.");
		return m_upvals[idx];
	}

	/// @brief sets the Upvalue at index [idx] in the upvalue list to the given Upvalue.
	void set_upval(u32 idx, Upvalue* uv);

	[[nodiscard]] size_t size() const override {
		return sizeof(Closure);
	}

  private:
	std::vector<Upvalue*> m_upvals;
	void trace(GC& gc) override;
};

/// TODO: Upvalues for CFunctions.

class CClosure final : public Obj {
  public:
	explicit CClosure(NativeFn fn, List* const values = nullptr) noexcept
		: Obj(ObjType::c_closure), m_values{values}, m_func{fn} {}
	~CClosure() override = default;

	[[nodiscard]] size_t size() const override {
		return sizeof(CClosure);
	}

	[[nodiscard]] NativeFn cfunc() const noexcept {
		return m_func;
	}

	/// @brief A list of values that the c-closure can use in whichever way it wants.
	List* m_values = nullptr;

  private:
	const NativeFn m_func;
	void trace(GC& gc) override;
};

} // namespace vy