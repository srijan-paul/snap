#include "util/test_utils.hpp"
#include <iostream>

using namespace vyse;

void strlib_test() {
	test_string_return("stdlib/substr.vy", "Moon", "String.substr");
	test_return("let s = 'ab' return byte(s[0]) * byte(s[1])", VYSE_NUM(97 * 98),
							"byte (char to ascii)");
	test_file("stdlib/map-str.vy", VYSE_NUM(597),
						"Mapping over characters of a string using closures.");
	std::cout << "[string lib tests passed]" << std::endl;
}

int main() {
	strlib_test();
	return 0;
}