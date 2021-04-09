#pragma once
#include "string.hpp"
#include "upvalue.hpp"

namespace snap {

// A protoype is the body of a function
// that contains the bytecode.
class Prototype final : public Obj {
public:
	u32 m_num_upvals = 0;
	Prototype(String* funcname) : Obj{ObjType::proto}, m_name{funcname} {};
	Prototype(String* funcname, u32 param_count)
			: Obj{ObjType::proto}, m_name{funcname}, m_num_params{param_count} {};

	const String* name() const;
	const char* name_cstr() const;

	Block& block();
	const Block& block() const;

	u32 add_param();
	u32 param_count() const;

	virtual size_t size() const override;
	virtual ~Prototype(){};

private:
	String* const m_name;
	u32 m_num_params = 0;
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
class Function final : public Obj {
public:
	Prototype* const m_proto;

	Function(Prototype* proto, u32 upval_count);

	const String* name() const;
	const char* name_cstr() const;

	/// @brief returns the Upvalue at index [idx] in the
	/// upvalue list.
	Upvalue* get_upval(u32 idx);
	/// @brief sets the Upvalue at index [idx] in the upvalue
	/// list to the given Upvalue.
	void set_upval(u32 idx, Upvalue* uv);

	virtual size_t size() const override;
	virtual ~Function(){};

private:
	std::vector<Upvalue*> m_upvals;

	void trace(GC& gc) override;
};

} // namespace snap