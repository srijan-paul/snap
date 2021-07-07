#pragma once
#include "function.hpp"
#include <common.hpp>
#include <dino/dino.hpp>
#include <forward.hpp>
#include <string>
#include <unordered_map>

namespace vyse {

struct StdModule;

class DynLoader final {
	VYSE_NO_MOVE(DynLoader);
	VYSE_NO_COPY(DynLoader);

  public:
	using Lib = dino::lib;

	DynLoader() = default;

	/// @brief Load the module loader functions into the VM's global variable table.
	/// These loader functions are used by the 'import' builtin
	void init_loaders(VM& vm) const;

	/// @brief Read a standard library module and return the value returned by it.
	Value read_std_lib(VM& vm, const StdModule& module);

	/// @brief A cached to avoid re-reading shared libraries that have already been read.
	/// module name -> module handle.
	std::unordered_map<std::string, Lib> cached_dyn_libs;
};

static constexpr const char* ModuleCacheName = "__modulecache__";
static constexpr const char* VMLoadersName = "__loaders__";

/// @brief The default module loader in vyse that only loads standard library modules like 'math'
Value load_std_module(VM& vm, int argc);

} // namespace vyse
