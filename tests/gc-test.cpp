#include "assert.hpp"
#include "function.hpp"
#include "util/test_utils.hpp"

using namespace vy;

#define ASSERT_MEM(got, expect, message)                                                           \
	ASSERT(got == expect, message << " (expected: " << expect << " got: " << got << ")");

static constexpr size_t table_size(int cap = Table::DefaultCapacity) {
	return sizeof(Table) + sizeof(Value) * cap;
}

static constexpr size_t string_size(int nchars) {
	return sizeof(char) * nchars + sizeof(vy::String);
}

static constexpr size_t closure_size = sizeof(vy::Closure);
static constexpr size_t proto_size = sizeof(vy::CodeBlock);

/// @brief The minimum memory allocated by a VM after having compiled any code.
static constexpr size_t base_size = closure_size + proto_size + string_size(8);

void test_gc() {
	VM vm;

	ASSERT_MEM(vm.memory(), 0, "0kb allocated before compilation is triggered.");

	vm.runcode("const empty_t = {}");
	ASSERT_MEM(vm.memory(), table_size() + base_size, "Empty table allocations.");
	vm.collect_garbage();

	vm.runcode("const s = 'abcdefg'");
	ASSERT_MEM(vm.memory(), base_size + closure_size + proto_size + string_size(7),
			   "String allocation test");
	vm.collect_garbage();
}

int main() {
	test_gc();
	printf("GC Tests successful.\n");
	return 0;
}
