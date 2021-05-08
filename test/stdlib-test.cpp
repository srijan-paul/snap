#include "util/test_utils.hpp"
#include <iostream>

using namespace vyse;

void strlib_test() {
	test_string_return("stdlib/substr.vy", "Moon", "String.substr");
	std::cout << "[string lib tests passed]" << std::endl;
}

int main() {
	strlib_test();
	return 0;
}