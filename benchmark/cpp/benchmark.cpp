#include "clock.hpp"
#include <vm.hpp>

using namespace snap;

/// @brief Runs a benchmark by running [code] [times] number of times (1000 by default)
/// then logs out the average time taken for [code] to run.
/// @param message An info message to display alongside the benchmark.
/// @param code   The snap code to run.
/// @param times  Number of times the code should run.
void run_code(std::string&& message, std::string&& code, std::size_t times = 10000) {
  Clock clk{message, times};
	for (std::size_t i = 0; i < times; i++) {
		VM vm{&code};
		vm.interpret();
	}
}

void run_benchmarks() {
	run_code("binary operators (+ - * /)", R"(
    1 + 2 + 3 * 4412 / 4 / 2
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
}

int main() {
	run_benchmarks();
	return 0;
}