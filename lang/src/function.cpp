#include <compiler.hpp>
#include <function.hpp>
#include <gc.hpp>
#include <upvalue.hpp>

namespace vyse {

u32 CodeBlock::add_param() {
	++m_num_params;
	VYSE_ASSERT(m_num_params < Compiler::MaxFuncParams, "Too many function parameters.");
	return m_num_params;
}

void CodeBlock::trace(GC& gc) {
	gc.mark_object(m_name);
	for (Value val : m_block.constant_pool) {
		gc.mark_value(val);
	}
}

/// Function `///

Closure::Closure(CodeBlock* code, u32 upval_count) noexcept
	: Obj(ObjType::closure), m_codeblock{code} {
	m_upvals.resize(upval_count);
}

void Closure::set_upval(u32 idx, Upvalue* uv) {
	VYSE_ASSERT(idx < m_upvals.size(), "Invalid upvalue index.");
	m_upvals[idx] = uv;
}

void Closure::trace(GC& gc) {
	for (Upvalue* upval : m_upvals) {
		gc.mark_object(upval);
	}
	gc.mark_object(m_codeblock);
}

void CClosure::trace([[maybe_unused]] GC& gc) {
	/// TODO: mark upvalues.
}

} // namespace vyse