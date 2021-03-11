#include <function.hpp>

namespace snap {

const String* Prototype::name() const {
	return m_name;
}

const char* Prototype::name_cstr() const {
	return m_name->c_str();
}

void Function::set_num_upvals(u32 count) {
	m_num_upvals = count;
	m_upvals.reserve(count);
}

const String* Function::name() const {
	return m_proto->name();
}

const char* Function::name_cstr() const {
	return m_proto->name_cstr();
}

Function::~Function() {
	for (u32 i = 0; i < m_num_upvals; ++i) {
		delete m_upvals[i];
	}
}

} // namespace snap