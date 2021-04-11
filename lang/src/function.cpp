#include <compiler.hpp>
#include <function.hpp>
#include <gc.hpp>
#include <iostream>
#include <upvalue.hpp>


namespace snap {

u32 Prototype::add_param() {
	++m_num_params;
	SNAP_ASSERT(m_num_params < Compiler::MaxFuncParams, "Too many function parameters.");
	return m_num_params;
}

void Prototype::trace(GC& gc) {
	gc.mark_object(m_name);
	for (Value val : m_block.constant_pool) {
		gc.mark_value(val);
	}
}

/// Function `///

Closure::Closure(Prototype* proto, u32 upval_count) noexcept : Obj(ObjType::func), m_proto{proto} {
	m_upvals.resize(upval_count);
}

void Closure::set_upval(u32 idx, Upvalue* uv) {
	SNAP_ASSERT(idx < m_upvals.size(), "Invalid upvalue index.");
	m_upvals[idx] = uv;
}

void Closure::trace(GC& gc) {
	for (Upvalue* upval : m_upvals) {
		gc.mark_object(upval);
	}
	gc.mark_object(m_proto);
}

} // namespace snap