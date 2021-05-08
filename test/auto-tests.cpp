#include "util/test_utils.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace stdfs = std::filesystem;

int main() {
	std::string dir_path = "../test/test_programs/auto";
	assert(stdfs::exists(dir_path) && "test directory exists.");

	for (const auto& entry : stdfs::directory_iterator(dir_path)) {
		if (entry.is_regular_file()) {
			std::ifstream stream{entry};
			std::ostringstream ostream;
			ostream << stream.rdbuf();
			runcode(ostream.str());
		}
	}

	std::cout << "auto tests passed" << std::endl;

	return 0;
}