#include "assert.hpp"
#include "test_utils.hpp"

using namespace snap;

/// Runs the next `op_count` instructions in the `code` string in a VM.
/// Then asserts that the value on top of the stack is equal to `expected_value`.
static void test_code(std::string&& code, int op_count, Value expected_value) {
	VM vm{&code};
	vm.init();
	vm.step(op_count);
	EXPECT_VAL_EQ(vm.peek(0), expected_value);
}

static void expr_tests() {
	test_code("let a = 5 / 2", 3, 2.5);
	test_code("let a = 5 % 2", 3, 1.0);
	test_code("let a = 5 + 2", 3, 7.0);
	test_code("let a = 5 - 2", 3, 3.0);
	test_code("let a = 3 * 5", 3, 15.0);
	test_code("let a = 10 - 5 - 2", 5, 3.0);
}

void vm_test() {
	std::printf("--- VM Tests ---\n");

	const std::string var_test = "let a = 1; let b = a + 2; a = 10;";
	VM vm2{&var_test};
	vm2.init();
	vm2.step(4);
	ASSERT_LOG(SNAP_IS_NUM(vm2.peek()) && SNAP_AS_NUM(vm2.peek()) == 3,
			   "Expected variable 'b' to have value '3'.");
	vm2.step(3);
	auto a = vm2.peek(1);
	ASSERT_LOG(VAL_NUM_EQ(a, 10), "Expected variable 'a' to have value '10'.");

	const std::string string_test = "let s1 = \"abcd\"; let s2 = 'ef'; let s3 = s1 .. s2;";
	VM vm3{&string_test};
	vm3.init();
	vm3.step();
	Value s = vm3.peek();
	ASSERT_LOG(s.is_string() && std::memcmp(SNAP_AS_CSTRING(s), "abcd", 5) == 0,
			   "Mismatched string value. Expected: "
				   << "'abcd' "
				   << "Got: '" << SNAP_AS_CSTRING(s) << "'");
	vm3.step(4);
	s = vm3.peek();
	ASSERT_LOG(s.is_string() && std::memcmp(SNAP_AS_CSTRING(s), "abcdef", 5) == 0,
			   "Mismatched string value. Expected: "
				   << "'abcdef' "
				   << "Got: '" << SNAP_AS_CSTRING(s) << "'");

	std::printf("--- /VM tests ---\n");
}

int main() {
	expr_tests();
	vm_test();
	return 0;
}