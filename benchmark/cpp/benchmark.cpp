#include "clock.hpp"
#include <cassert>
#include <fstream>
#include <sstream>
#include <vm.hpp>

using namespace vyse;

/// @brief Runs a benchmark by running [code] [times] number of times (1000 by default)
/// then logs out the average time taken for [code] to run.
/// @param message An info message to display alongside the benchmark.
/// @param code   The vyse code to run.
/// @param times  Number of times the code should run.
void run_code(std::string&& message, std::string&& code, std::size_t times = 10000) {
	Clock clk{message, times};
	for (std::size_t i = 0; i < times; i++) {
		VM vm{&code};
		ExitCode ec = vm.interpret();
		assert(ec == ExitCode::Success);
	}
}

void run_file(const char* filename, std::string&& message, std::size_t times = 1000) {
	static const char* path_prefix = "../benchmark/cpp/scripts/";
	std::string filepath{path_prefix};
	filepath += filename;

	std::ifstream file(filepath);

	if (file) {
		std::ostringstream stream;
		stream << file.rdbuf();
		run_code(std::move(message), stream.str(), times);
		return;
	}

	fprintf(stderr, "Could not open file '%s'", filepath.c_str());
	abort();
}

void run_benchmarks() {
	run_code("binary operators (+ - * /)", R"(
    return 1 + 2 + 3 * 4412 / 4 / 2
  )");

	run_code("string creation and operations", R"(
    const a = "hi this is a string"
    const b = " and this is another string"
    const c = a .. b
    let temp = "concatenating".."some stringsss"
  )");

	run_code("recursive fibonacci call", R"(
    fn fib(n) {
      if n <= 1 return 1
      return fib(n - 1) + fib(n - 2)
    }
    fib(10)
  )");

	run_file("table.snp", "table insertion and field access.");
}

int main() {
	run_benchmarks();
	return 0;
}