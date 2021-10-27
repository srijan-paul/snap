#pragma once
#include "function.hpp"
#include <common.hpp>
#include <dino/dino.hpp>
#include <forward.hpp>
#include <string>
#include <unordered_map>

namespace vyse {

struct StdModule;

static constexpr const char* ModuleCacheName = "__modulecache__";
static constexpr const char* VMLoadersName = "__loaders__";
static constexpr const char* VyseEnvVar = "VYSE_PATH";

class DynLoader final {
	VYSE_NO_MOVE(DynLoader);
	VYSE_NO_COPY(DynLoader);

  public:
	using Lib = dino::lib;

	DynLoader() : std_dlls_path(getenv(VyseEnvVar)) {}

	/// @brief Load the module loader functions into the VM's global variable table.
	/// These loader functions are used by the 'import' builtin
	void init_loaders(VM& vm) const;

	/// @brief Read a standard library module and return the value returned by it.
	Value read_std_lib(VM& vm, const StdModule& module);

	/// @brief A cached to avoid re-reading shared libraries that have already been read.
	/// module name -> module handle.
	std::unordered_map<std::string, Lib> cached_dyn_libs;

	/// @brief the path to the directory where all the standard library shared modules
	/// are placed. This is extracted via the VYSE_PATH environment variable.
	const char* std_dlls_path;
};

/// @brief The default module loader in vyse that only loads standard library modules like 'math'
Value load_std_module(VM& vm, int argc);

} // namespace vyse

