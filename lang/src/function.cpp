#include "upvalue.hpp"
#include "value.hpp"
#include <function.hpp>
#include <iostream>

namespace snap {

const String* Prototype::name() const {
	return m_name;
}

const char* Prototype::name_cstr() const {
	return m_name->c_str();
}

Block& Prototype::block() {
	return m_block;
}

const Block& Prototype::block() const {
	return m_block;
}

u32 Prototype::add_param() {
	return ++m_num_params;
}

u32 Prototype::param_count() const {
	return m_num_params;
}

void Prototype::trace(GC& gc) {
}

/// Function ///

Function::Function(const Prototype* proto, u32 upval_count): Obj(ObjType::func), m_proto{proto} {
	m_upvals.resize(upval_count);
}

Upvalue* Function::get_upval(u32 idx) {
	return m_upvals[idx];
}

void Function::set_upval(u32 idx, Upvalue* uv) {
	SNAP_ASSERT(idx < m_upvals.size(), "Invalid upvalue index.");
	m_upvals[idx] = uv;
}

const String* Function::name() const {
	return m_proto->name();
}

const char* Function::name_cstr() const {
	return m_proto->name_cstr();
}

void Function::trace(GC& gc) {

}

} // namespace snap