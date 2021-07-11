#include "loadlib.hpp"
#include "std/lib_util.hpp"
#include <common.hpp>
#include <cstdlib>
#include <dino/dino.hpp>
#include <forward.hpp>
#include <string>
#include <vm.hpp>
#include <vy_list.hpp>

namespace vyse {

struct StdModule final {
	/// @brief The module name that the user has to use when calling `import` or `require`
	const char* module_name;
	/// @brief Name of the shared library storing this module. e.g -> libmodule.dll or libmodule.so
	const char* dll_name;
};

Value DynLoader::read_std_lib(VM& vm, const StdModule& module) {
	if (not std_dlls_path) return VYSE_NIL;

	std::string init_func_name = std::string("load_") + module.module_name;
	std::string dll_name{module.dll_name};

	auto cached_it = cached_dyn_libs.find(dll_name);
	if (cached_it == cached_dyn_libs.end()) {
		Lib lib(dll_name, std_dlls_path);
		cached_dyn_libs.emplace(dll_name, std::move(lib));
	}

	const Lib& lib = cached_dyn_libs.find(dll_name)->second;
	if (!lib) return VYSE_NIL;

	if (auto init_lib = lib.find<void(VM*, Table*)>(init_func_name)) {
		Table& t = vm.make<Table>();
		init_lib(&vm, &t);
		return VYSE_OBJECT(&t);
	}

	return VYSE_NIL;
}

static constexpr std::array<StdModule, 1> std_modules = {{
#ifdef _WIN32
	{"math", "libvymath"},
#else
	{"math", "vymath"}
#endif
}};

Value load_std_module(VM& vm, int argc) {
	if (argc != 1) return VYSE_NIL;
	if (!VYSE_IS_STRING(vm.get_arg(0))) return VYSE_NIL;

	/// name of the standard library module
	const String& modname = *VYSE_AS_STRING(vm.get_arg(0));

	for (const auto& module : std_modules) {
		if (strcmp(module.module_name, modname.c_str()) == 0) {
			return vm.dynloader.read_std_lib(vm, module);
		}
	}

	return VYSE_NIL;
}

/// @brief Checks if a module is cached and returns the cached module if present.
Value load_cached_module(VM& vm, int argc) {
	if (argc != 1) return VYSE_NIL;

	Value modname = vm.get_arg(0);
	if (!VYSE_IS_STRING(modname)) return VYSE_NIL;

	Value vcache = vm.get_global(ModuleCacheName);
	if (!VYSE_IS_TABLE(vcache)) return VYSE_NIL;

	Table& cache = *VYSE_AS_TABLE(vcache);
	Value cached_module = cache.get(modname);
	return cached_module;
}

void DynLoader::init_loaders(VM& vm) const {
	// A list of loader functions that the VM calls one after the other to find a given module until
	// the module is found.
	List& loaders = vm.make<List>();
	vm.set_global("__loaders__", VYSE_OBJECT(&loaders));

	Table& cache = vm.make<Table>();
	vm.set_global(ModuleCacheName, VYSE_OBJECT(&cache));

	// First a lookup is performed on the cache to check if the module has already been loaded.
	CClosure& cached_lib_loader = vm.make<CClosure>(load_cached_module);
	loaders.append(VYSE_OBJECT(&cached_lib_loader));

	// Add the default standard library module loader.
	CClosure& stdlib_loader = vm.make<CClosure>(load_std_module);
	loaders.append(VYSE_OBJECT(&stdlib_loader));
}

} // namespace vyse

