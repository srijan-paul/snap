#pragma once
#include <common.hpp>
#include <forward.hpp>
#include <gc.hpp>

namespace vyse::util {

class NativeModule {
  public:
	VYSE_NO_COPY(NativeModule);
	VYSE_NO_MOVE(NativeModule);
	VYSE_NO_DEFAULT_CONSTRUCT(NativeModule);

	NativeModule(VM* vm, Table* table);
	~NativeModule() = default;

	void add_cfunc(const char* fname, NativeFn func);
	void add_field(const char* name, Value value);

  private:
	VM* const m_vm;
	Table* const m_table;

	/// @brief a lock on the table to prevent it from getting
	/// destroyed as long as this module object is alive.
	GCLock m_lock;
};

} // namespace vyse::util