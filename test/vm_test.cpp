#include "assert.hpp"
#include "test_utils.hpp"
#include "value.hpp"
#include <fstream>
#include <memory>
#include <sstream>
#include <stdlib.h>

using namespace snap;

void assert_val_eq(Value& expected, Value actual, const char* message = "Test failed! ") {
	if (expected != actual) {
		fprintf(stderr, "%s: ", message);
		fprintf(stderr, "Expected value to be ");
		print_value(expected);
		fprintf(stderr, " But got  ");
		print_value(actual);
		abort();
	}
}

static void test_return(const std::string&& code, Value expected,
						const char* message = "Test failed!") {
	VM vm{&code};
	vm.interpret();
	assert_val_eq(expected, vm.return_value, message);
}

/// Given the name of a test file to run, checks the value returned by that file's evaluation.
/// @param filename name of the file, must be inside 'test/test_programs/'.
/// @param expected Expected return value once the file is interpreted.
/// @param message Message to be displayed on failure.
static void test_file(const char* filename, Value expected, const char* message = "Failure") {

	// Currently files can only be read relative to the path of the binary (which is
	// in `bin/vm_test`).
	// So to read the file just from it's name, we append the file beginning of the
	// file path to it.
	static const char* path_prefix = "../test/test_programs/";
	std::string filepath{path_prefix};
	filepath += filename;

	std::ifstream file(filepath);

	if (file) {
		std::ostringstream stream;
		stream << file.rdbuf();
		test_return(stream.str(), expected, message);
		return;
	}

	fprintf(stderr, "Could not open file '%s'", filepath.c_str());
	abort();
}

/// Runs the next `op_count` instructions in the `code` string in a VM.
/// Then asserts that the value on top of the stack is equal to `expected_value`.
/// If `op_count` is -1 then interprets the entire bytecode and checks the return value.
static void test_code(const std::string&& code, int op_count, Value expected_value) {
	VM vm{&code};
	if (op_count == -1) {
		vm.interpret();
		assert_val_eq(expected_value, vm.return_value);
	} else {
		vm.init();
		vm.step(op_count);
		assert_val_eq(expected_value, vm.peek());
	}
}

static void expr_tests() {
	test_code("5 / 2", 3, SNAP_NUM_VAL(2.5));
	test_code("5 % 2", 3, SNAP_NUM_VAL(1.0));
	test_code("5 + 2", 3, SNAP_NUM_VAL(7.0));
	test_code("5 - 2", 3, SNAP_NUM_VAL(3.0));
	test_code("3 * 5", 3, SNAP_NUM_VAL(15.0));

	test_code("3 < 5", 3, SNAP_BOOL_VAL(true));
	test_code("3 > 5", 3, SNAP_BOOL_VAL(false));
	test_code("3 <= 3", 3, SNAP_BOOL_VAL(true));
	test_code("4 >= 4", 3, SNAP_BOOL_VAL(true));

	test_code("10 - 5 - 2", 5, SNAP_NUM_VAL(3.0));
	test_code("let a = 10 && 5 && 2", 5, SNAP_NUM_VAL(2));
	test_code("let a = 10 || 5 || 2", 2, SNAP_NUM_VAL(10));
	test_code("10 - 5 - 2", 5, SNAP_NUM_VAL(3.0));

	test_code(R"(
		let a = 1
		let b = 3
		a = b = 5
		let c = a + b
	)",
			  8, 5.0);

	test_code("4 | 9", 3, SNAP_NUM_VAL(13));
	test_code("9 & 7", 3, SNAP_NUM_VAL(1));

	// test precedence

	test_code("let a = 5 > 2 && 3 > -10", 8, SNAP_BOOL_VAL(true));

	// test string equality
	test_return("return 'abc' == 'abc'", SNAP_BOOL_VAL(true));
	test_return(R"(
		const a = 'abcde'
		const b = "abcde"
		const c = 'aa'
		const d = 'bb'

		return a == b and c..d == 'aabb'
	)",
				SNAP_BOOL_VAL(true));

	test_file("compound-assign.snp", SNAP_NUM_VAL(8), "Compound assignment operators.");
}

static void stmt_tests() {
	test_code(R"(
		let a = 1;
		let b = 2;
		if a < b {
			b = 5
		})",
			  6, SNAP_NUM_VAL(2));

	test_code(R"(
		let a = 3;
		let b = 3;
		if a < b {
			b = 5
		} else if b > a {
			b = 6
		} else {
			b = 7
		}
		return 7
	)",
			  -1, SNAP_NUM_VAL(7));
}

void fn_tests() {
	test_return(R"(
		fn adder(x) {
			fn add(y) {
				return x + y
			}	
			return add
		}
		return adder(10)(20)
	)",
				SNAP_NUM_VAL(30));

	test_return(R"(
		fn x() {
			let a = 1
			let b = 2
			fn y() {
				let c = 3
				fn z() {
					return a + b + c;
				}
				return z
			}
			return y
		}
		return x()()()
	)",
				SNAP_NUM_VAL(6));

	test_return(R"(
		fn fib(n) {
			if (n <= 1) return 1
			return fib(n - 1) + fib(n - 2)
		}
		return fib(10)
	)",
				SNAP_NUM_VAL(89));

	test_return(R"(
		fn make_adder(x) {
			return fn(y) {
				return x + y
			}
		}

		const add10 = make_adder(10)
		return add10(-10)
	)",
				SNAP_NUM_VAL(0));

	test_file("llnode-cl.snp", SNAP_NUM_VAL(20), "Linked list closure test");
}

void table_test() {
	test_return(R"(
		fn Node(a, b) {
			return  { data: a, next: b }
		}
		const head = Node(10, Node(20))
		return head.next.data
	)",
				SNAP_NUM_VAL(20), "Linked lists as tables test");

	// setting table field names.
	test_return(R"(
		const t = {
			k : 7 
		}
		let a = t.k
		t.k = 3
		return t.k +  a
	)",
				SNAP_NUM_VAL(10));

	test_file("table-1.snp", SNAP_NUM_VAL(3), "returning tables from a closure.");
	test_file("table-2.snp", SNAP_NUM_VAL(6), "Accessing table fields");
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
	stmt_tests();
	fn_tests();
	table_test();
	vm_test();
	return 0;
}