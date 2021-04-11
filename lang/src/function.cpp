#include <function.hpp>
#include <gc.hpp>
#include <iostream>
#include <upvalue.hpp>
#include <compiler.hpp>

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

/// Function ///

Function::Function(Prototype* proto, u32 upval_count) : Obj(ObjType::func), m_proto{proto} {
	m_upvals.resize(upval_count);
}

void Function::set_upval(u32 idx, Upvalue* uv) {
	SNAP_ASSERT(idx < m_upvals.size(), "Invalid upvalue index.");
	m_upvals[idx] = uv;
}

void Function::trace(GC& gc) {
	for (Upvalue* upval : m_upvals) {
		gc.mark_object(upval);
	}
	gc.mark_object(m_proto);
}

} // namespace snap