#include "function.hpp"
#include "gc.hpp"
#include <util/native_module.hpp>
#include <vm.hpp>

namespace vy::util {

NativeModule::NativeModule(VM* vm, Table* table)
	: m_vm{vm}, m_table{table}, m_lock{vm->gc_lock(table)} {}

void NativeModule::add_cfunc(const char* name, NativeFn func) {
	m_vm->gc_off();
	String* sname = &m_vm->make_string(name);
	CClosure* fn = &m_vm->make<CClosure>(func);
	m_table->set(VYSE_OBJECT(sname), VYSE_OBJECT(fn));
	m_vm->gc_on();
}

void NativeModule::add_cclosures(const std::pair<const char*, NativeFn>* funcs,
								 std::size_t num_funcs) {
	m_vm->gc_off();
	for (uint i = 0; i < num_funcs; ++i) {
		auto [fn_name, fn] = funcs[i];
		String& name = m_vm->make_string(fn_name);
		CClosure& ccl = m_vm->make<CClosure>(fn);
		m_table->set(VYSE_OBJECT(&name), VYSE_OBJECT(&ccl));
	}
	m_vm->gc_on();
}

void NativeModule::add_field(const char* name, Value value) {
	String& vyname = m_vm->make_string(name);
	GCLock lock = m_vm->gc_lock(&vyname);
	m_table->set(vyname, value);
}

} // namespace vy::util
