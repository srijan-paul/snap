#include "upvalue.hpp"
#include <function.hpp>

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

Upvalue* Function::get_upval(u32 idx) {
	return m_upvals[idx];
}

void Function::set_num_upvals(u32 count) {
	m_num_upvals = count;
	m_upvals.reserve(count);
}

void Function::set_upval(u32 idx, Upvalue* uv) {
	m_upvals[idx] = uv;
}

const String* Function::name() const {
	return m_proto->name();
}

const char* Function::name_cstr() const {
	return m_proto->name_cstr();
}

} // namespace snap