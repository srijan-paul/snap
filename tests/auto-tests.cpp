#include "util/test_utils.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vm.hpp>

namespace stdfs = std::filesystem;

int main() {
	std::string dir_path = "../tests/test_programs/auto";
	assert(stdfs::exists(dir_path) && "test directory exists.");

	auto run_code = [](std::string fpath, std::string code) {
		vy::VM vm;
		vm.load_stdlib();
		vy::ExitCode ec = vm.runfile(fpath, code);
		if (ec != vy::ExitCode::Success) {
			std::cerr << "Failure running auto test ("  << fpath << "). " << std::endl;
			abort();
		}
	};

	for (const auto& entry : stdfs::directory_iterator(dir_path)) {
		if (entry.is_regular_file()) {
			std::cout << "[Running test] " << entry.path().filename() << " ... ";
			std::ifstream stream(entry.path());
			std::ostringstream ostream;
			ostream << stream.rdbuf();
			run_code(entry.path().string(), ostream.str());
			std::cout << " [DONE]\n";
		}
	}

	std::cout << "auto tests passed" << std::endl;
	return 0;
}
