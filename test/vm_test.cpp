#include "test_utils.hpp"
#include "assert.hpp"

using namespace snap;

void vm_test() {
	std::printf("--- VM Tests ---\n");

	const std::string code = "3 * 5";
	VM vm{&code};
	vm.init();
	vm.step(3);
	EXPECT_VAL_EQ(vm.peek(0), SNAP_INT_VAL(15));

	const std::string var_test = "let a = 1; let b = a + 2; a = 10;";
	VM vm2{&var_test};
	vm2.init();
	vm2.step(4);
	ASSERT_LOG(vm2.peek().is_int() && vm2.peek().as_int() == 3,
		   "Expected variable 'b' to have value '3'.");
	vm2.step(3);
	auto a = vm2.peek(1);
	ASSERT_LOG(VAL_INT_EQ(a, 10), "Expected variable 'a' to have value '10'.");

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
	vm_test();
	return 0;
}