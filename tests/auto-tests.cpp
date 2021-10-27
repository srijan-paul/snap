#include "util/test_utils.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace stdfs = std::filesystem;

int main() {
	std::string dir_path = "../tests/test_programs/auto";
	assert(stdfs::exists(dir_path) && "test directory exists.");

	for (const auto& entry : stdfs::directory_iterator(dir_path)) {
		if (entry.is_regular_file()) {
			std::cout << "[Running test] " << entry.path().filename() << " ... ";
			std::ifstream stream(entry.path());
			std::ostringstream ostream;
			ostream << stream.rdbuf();
			runcode(ostream.str());
			std::cout << " [DONE]\n";
		}
	}

	std::cout << "auto tests passed" << std::endl;
	return 0;
}
