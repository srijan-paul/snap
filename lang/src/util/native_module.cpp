#include "gc.hpp"
#include <util/native_module.hpp>
#include <vm.hpp>

namespace vyse::util {

NativeModule::NativeModule(VM* vm, Table* table)
	: m_vm{vm}, m_table{table}, m_lock{vm->gc_lock(table)} {}

void NativeModule::add_cfunc(const char* name, NativeFn func) {
	m_vm->gc_off();
	String* sname = &m_vm->make_string(name);
	CClosure* fn = &m_vm->make<CClosure>(func);
	m_table->set(VYSE_OBJECT(sname), VYSE_OBJECT(fn));
	m_vm->gc_on();
}

void NativeModule::add_field(const char* name, Value value) {
	String& vyname = m_vm->make_string(name);
	GCLock lock = m_vm->gc_lock(&vyname);
	m_table->set(vyname, value);
}

} // namespace vyse::util