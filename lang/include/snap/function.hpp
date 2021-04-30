#pragma once
#include "string.hpp"
#include "upvalue.hpp"

namespace snap {

// A protoype is the body of a function
// that contains the bytecode.
class CodeBlock final : public Obj {
	friend Compiler;

public:
	explicit CodeBlock(String* funcname) noexcept : Obj{ObjType::codeblock}, m_name{funcname} {};
	explicit CodeBlock(String* funcname, u32 param_count) noexcept
			: Obj{ObjType::codeblock}, m_name{funcname}, m_num_params{param_count} {};

	~CodeBlock(){};

	const String* name() const {
		return m_name;
	}

	const char* name_cstr() const {
		return m_name->c_str();
	}

	Block& block() {
		return m_block;
	}

	const Block& block() const {
		return m_block;
	}

	u32 add_param();

	u32 param_count() const {
		return m_num_params;
	}

	size_t size() const override {
		return sizeof(CodeBlock);
	}

private:
	String* const m_name;
	u32 m_num_params = 0;
	u32 m_num_upvals = 0;
	// the maximum stack size ever needed for the 
	// execution of this function's call frame.
	size_t max_stack_size = 0;
	Block m_block;

	void trace(GC& gc) override;
};

/// @brief The Closure object.
/// A closure has two parts, code and data.
/// The code part is represented by the prototype
/// containing all the bytecode instructions and
/// the data part is represented by the upvalues vector
/// holding all the captured variables from enclosing
/// scopes.
class Closure final : public Obj {
public:
	CodeBlock* const m_codeblock;

	explicit Closure(CodeBlock* proto, u32 upval_count) noexcept;
	~Closure() override{};

	const String* name() const {
		return m_codeblock->name();
	}

	const char* name_cstr() const {
		return m_codeblock->name_cstr();
	}

	/// @brief returns the Upvalue at index [idx] in the
	/// upvalue list.
	Upvalue* get_upval(u32 idx) noexcept {
		SNAP_ASSERT(idx < m_upvals.size(), "Invalid upvalue index.");
		return m_upvals[idx];
	}

	/// @brief sets the Upvalue at index [idx] in the upvalue
	/// list to the given Upvalue.
	void set_upval(u32 idx, Upvalue* uv);

	size_t size() const override {
		return sizeof(Closure);
	}

private:
	std::vector<Upvalue*> m_upvals;
	void trace(GC& gc) override;
};

/// TODO: Upvalues for CFunctions.

class CClosure final : public Obj {
public:
	explicit CClosure(CFunction fn) noexcept : Obj(ObjType::c_closure), m_func{fn} {};
	~CClosure() override{};

	size_t size() const override {
		return sizeof(CClosure);
	}

	CFunction cfunc() const noexcept {
		return m_func;
	}

private:
	CFunction m_func;
	void trace(GC& gc) override;
};

} // namespace snap