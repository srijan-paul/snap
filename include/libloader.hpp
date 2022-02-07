#pragma once
#include <common.hpp>
#include <dino/dino.hpp>
#include <forward.hpp>
#include <unordered_map>

namespace vy {

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

  private:
	/// @brief A cache to avoid re-reading (.dll/.so/.a)s that have already been read.
	/// Map of module name -> module handle.
	std::unordered_map<std::string, Lib> cached_dyn_libs;

	/// @brief Path to the directory where all the standard library shared modules
	/// are placed. This is extracted via the VYSE_PATH environment variable.
	std::string std_dlls_path;
};


} // namespace vy
