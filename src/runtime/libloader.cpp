#include "source.hpp"
#include "util/args.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <libloader.hpp>
#include <list.hpp>
#include <string_view>
#include <util/lib_util.hpp>
#include <vm.hpp>
#include "../str_format.hpp"

namespace vy {

struct StdModule final {
	/// @brief The module name that the user has to use when calling `import` or `require`
	const char* module_name;
	/// @brief Name of the shared library storing this module. e.g -> libmodule.dll or libmodule.so
	const char* dll_name;
};

Value DynLoader::read_std_lib(VM& vm, const StdModule& module) {
	if (std_dlls_path.empty()) return VYSE_NIL;

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
	{"math", "vymath"},
#endif
}};

Value load_std_module(VM& vm, int argc) {
	util::Args args{vm, "load_std_module", 1, argc};
	const String& modname = args.next<String>();

	for (const StdModule& module : std_modules) {
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

std::string resolve_abs_path(std::string_view current_file, std::string_view import_file) {
	std::filesystem::path current_path{current_file};
	std::filesystem::path import_path{import_file};

	if (current_path.empty() || import_path.empty()) {
		return "";
	}

	current_path = current_path.parent_path();
	return current_path / import_path;
}


static bool is_recursive_import(const std::vector<SourceCode>& sources, const std::string& current_file) {
	const std::filesystem::path p_current_source{current_file};
	for (const auto& source : sources) {
		if (source.path.empty()) continue;
		const std::filesystem::path src_path{source.path};
		if (src_path.compare(p_current_source) == 0) {
			return true;
		}
	}
	return false;
}

Value load_module_from_fs(VM& vm, int argc) {
	util::Args args{vm, "load_module_from_fs", 1, argc};
	String& module_path = args.next<String>();

	const std::string module_path_s{module_path.c_str()};
	const std::string_view current_path = vm.get_current_file();

	const std::string resolved_module_path = resolve_abs_path(current_path, module_path_s);
	if (resolved_module_path.empty()) return VYSE_NIL;

	const bool is_recursive = is_recursive_import(vm.m_sources, resolved_module_path);
	args.check(!is_recursive, kt::format_str("recursive import detected: '{}'", module_path.c_str()));

	auto maybe_source = SourceCode::from_path(resolved_module_path);
	if (!maybe_source.has_value()) return VYSE_NIL;
	
	Closure* file_func = vm.compile(maybe_source.value());
	vm.ensure_slots(1);
	vm.m_stack.push(VYSE_OBJECT(file_func));
	vm.call(0);

	vm.pop_source();
	Value exported = vm.m_stack.pop();

	std::string error_message = kt::format_str("No exports found in file '{}'", module_path_s);
	args.check(!VYSE_IS_NIL(exported), error_message.c_str());
	return exported;
}

void DynLoader::init_loaders(VM& vm) const {
	// A list of loader functions that the VM calls one after the other to find a given module until
	// the module is found.
	List& loaders = vm.make<List>();
	vm.set_global("__loaders__", VYSE_OBJECT(&loaders));

	Table& cache = vm.make<Table>();
	vm.set_global(ModuleCacheName, VYSE_OBJECT(&cache));

	// First, a lookup is performed on the cache to check if the module has already been loaded.
	// This "cache search" is done by the `cached_lib_loader` closure.
	CClosure& cached_lib_loader = vm.make<CClosure>(load_cached_module);
	loaders.append(VYSE_OBJECT(&cached_lib_loader));

	// Second step is to attempt to load a standard library module.
	CClosure& stdlib_loader = vm.make<CClosure>(load_std_module);
	loaders.append(VYSE_OBJECT(&stdlib_loader));

	// Third step is to attempt to load a vyse file from the filesystem.
	CClosure& fs_loader = vm.make<CClosure>(load_module_from_fs);
	loaders.append(VYSE_OBJECT(&fs_loader));
}

} // namespace vy
