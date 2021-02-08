#pragma once
#include "string.hpp"
#include "upvalue.hpp"

namespace snap {

// A protoype is the body of a function
// that contains the bytecode.
struct Prototype : Obj {
	const String* m_name;
	u32 m_num_params = 0;
	u32 m_num_upvals = 0;
	Block m_block;
	Prototype(const String* funcname) : Obj{ObjType::proto}, m_name{funcname} {};
	Prototype(const String* funcname, u32 param_count)
		: Obj{ObjType::proto}, m_name{funcname}, m_num_params{param_count} {};

	const String* name() const;
	const char* name_cstr() const;
};

// The "Closure"
struct Function : Obj {
	Prototype* m_proto;
	std::vector<Upvalue*> m_upvals;
	u32 m_num_upvals = 0;
	Function(String* name) : Obj(ObjType::func), m_proto{new Prototype(name)} {};
	Function(Prototype* proto_) : Obj(ObjType::func), m_proto{proto_} {};

	void set_num_upvals(u32 count);
	const String* name() const;
	const char* name_cstr() const;
	~Function();
};

} // namespace snap