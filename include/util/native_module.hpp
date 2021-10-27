#pragma once
#include <common.hpp>
#include <forward.hpp>
#include <gc.hpp>

namespace vyse::util {

/// @brief A NativeModule helper class to manage native extensions to vyse.
/// Check `lang/src/vymath.cpp` for usage reference.
class NativeModule {
  public:
	VYSE_NO_COPY(NativeModule);
	VYSE_NO_MOVE(NativeModule);
	VYSE_NO_DEFAULT_CONSTRUCT(NativeModule);

	NativeModule(VM* vm, Table* table);
	~NativeModule() = default;

	/// @brief Add a CClosure wrapping `func` to the table with field-name `name`.
	void add_cfunc(const char* fname, NativeFn func);
	void add_field(const char* name, Value value);

	/// @brief add `num_funcs` functions from the `funcs` list to the module's table.
	/// @param funcs An array of pairs of (name, function_pointer) where `name` represents the name
	/// of the function and `function_pointer` is a pointer to the C function that will be exposed
	/// via the module
	void add_cclosures(const std::pair<const char*, NativeFn>* funcs, std::size_t num_funcs);

  private:
	VM* const m_vm;
	Table* const m_table;

	/// @brief a lock on the table to prevent it from getting
	/// destroyed as long as this module object is alive.
	const GCLock m_lock;
};

} // namespace vyse::util