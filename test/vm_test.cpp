#include "assert.hpp"
#include "test_utils.hpp"
#include "value.hpp"
#include "vm.hpp"
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
		fprintf(stderr, " But got ");
		print_value(actual);
		abort();
	}
}

static void test_return(const std::string&& code, Value expected,
												const char* message = "Test failed!") {
	VM vm;
	vm.runcode(code);
	assert_val_eq(expected, vm.return_value, message);
}

static std::string load_file(const char* filename) {
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
		return stream.str();
	}

	fprintf(stderr, "Could not open file '%s'", filepath.c_str());
	abort();
}

/// Given the name of a test file to run, checks the value returned by that file's evaluation.
/// @param filename name of the file, must be inside 'test/test_programs/'.
/// @param expected Expected return value once the file is interpreted.
/// @param message Message to be displayed on failure.
static void test_file(const char* filename, Value expected, const char* message = "Failure") {
	test_return(load_file(filename), expected, message);
	return;
}

/// @brief Runs `filename` and asserts the return value as a cstring, comparing
/// it with `expected`.
static void test_string_return(const char* filename, const char* expected,
															 const char* message = "Failed") {
	std::string code{load_file(filename)};
	VM vm;

	if (vm.runcode(code) != ExitCode::Success) {
		std::cout << message << "\n";
		abort();
	}

	if (!SNAP_IS_STRING(vm.return_value)) {
		std::cout << "[ Failed ]" << message << "Expected string but got ";
		print_value(vm.return_value);
		std::cout << "\n";
		abort();
	}

	if (std::memcmp(SNAP_AS_CSTRING(vm.return_value), expected, strlen(expected)) != 0) {
		std::cout << message << "[ STRINGS NOT EQUAL ] \n";
		std::cout << "Expected: '" << expected << "'.\n";
		std::cout << "Got: '" << SNAP_AS_CSTRING(vm.return_value) << "'\n";
		abort();
	}
}

static void expr_tests() {
	test_return("return 5 / 2", SNAP_NUM_VAL(2.5));
	test_return("return 5 % 2", SNAP_NUM_VAL(1.0));
	test_return("return 5 + 2", SNAP_NUM_VAL(7.0));
	test_return("return 5 - 2", SNAP_NUM_VAL(3.0));
	test_return("return 3 * 5", SNAP_NUM_VAL(15.0));

	test_return("return 3 < 5", SNAP_BOOL_VAL(true));
	test_return("return 3 > 5", SNAP_BOOL_VAL(false));
	test_return("return 3 <= 3", SNAP_BOOL_VAL(true));
	test_return("return 4 >= 4", SNAP_BOOL_VAL(true));

	test_return("return 10 - 5 - 2", SNAP_NUM_VAL(3.0));
	test_return("return 10 && 5 && 2", SNAP_NUM_VAL(2));
	test_return("return 10 || 5 || 2", SNAP_NUM_VAL(10));
	test_return("return 10 - 5 - 2", SNAP_NUM_VAL(3.0));

	test_return(R"(
		let a = 1
		let b = 3
		a = b = 5
		return a + b
	)",
							SNAP_NUM_VAL(10.0), "Chained assignments to local variables");

	test_return("return 9 & 7", SNAP_NUM_VAL(1));
	test_return("return 4 | 9", SNAP_NUM_VAL(13));

	// test precedence

	test_return("return 5 > 2 && 3 > -10", SNAP_BOOL_VAL(true));

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

	test_file("compound-assign.snp", SNAP_NUM_VAL(8), "Compound assignment operators");

	std::cout << "[Expression tests passed]\n";
}

static void stmt_tests() {
	test_return(R"(
		let a = 1;
		let b = 2;
		if a < b {
			b = 5
		}
		return b
		)",
							SNAP_NUM_VAL(5), "If statement without else branch");

	test_return(R"(
		let a = 3;
		let b = 3;
		if a < b {
			b = 5
		} else if b > a {
			b = 6
		} else {
			b = 7
		}
		return b)",
							SNAP_NUM_VAL(7), "If statement with else-if branch");

	std::cout << "Statement tests passed\n";
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
							SNAP_NUM_VAL(30), "chained calls with parameters");

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
							SNAP_NUM_VAL(6), "Closures and chained function calls");

	test_return(R"(
		fn fib(n) {
			if (n <= 1) return 1
			return fib(n - 1) + fib(n - 2)
		}
		return fib(10)
	)",
							SNAP_NUM_VAL(89), "Recursive fiboacci");

	test_return(R"(
		fn make_adder(x) {
			return fn(y) {
				return x + y
			}
		}

		const add10 = make_adder(10)
		return add10(-10)
	)",
							SNAP_NUM_VAL(0), "make_adder closure");

	test_file("llnode-cl.snp", SNAP_NUM_VAL(20), "Linked list closure test");
}

void table_test() {
	test_return(R"(
	const T = 	{
		a: 1, b : 2
	} 
	return T.a + T.b
	)",
							SNAP_NUM_VAL(3), " Indexing tables with '.' operator");

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
							SNAP_NUM_VAL(10), "setting table field names");

	test_file("table-1.snp", SNAP_NUM_VAL(3), "returning tables from a closure");
	test_file("table-2.snp", SNAP_NUM_VAL(6), "Accessing table fields");
	test_file("table-3.snp", SNAP_NUM_VAL(11), "Accessing table fields");
	test_file("table-4.snp", SNAP_NUM_VAL(10), "Computed member assignment and access");
	test_file("table-5.snp", SNAP_NUM_VAL(10),
						"Computed member access and dot member access are equivalent for string keys");
	test_file("table-6.snp", SNAP_NUM_VAL(25), "Compound assignment to computed members");
	test_file("table-7.snp", SNAP_NUM_VAL(10), "Syntactic sugar for table methods");
	test_file("table-8.snp", SNAP_NUM_VAL(1), "Empty tables.");
	test_file("table-9.snp", SNAP_NUM_VAL(30), "Chained dot and subscript operators.");
	test_file("keys.snp", SNAP_NUM_VAL(6), "Subscript operator in table key with interned strings");
	test_file("point.snp", SNAP_NUM_VAL(50), "Constructor like functions. (point.snp)");
	test_file("method-1.snp", SNAP_NUM_VAL(3), "Method call syntax.");
	test_file("method-2.snp", SNAP_NUM_VAL(5), "Chained method calls.");
	test_file("suffix-expr.snp", SNAP_NUM_VAL(123), "Chained suffix expressions.");

	std::cout << "[Table tests passed]\n";
}

void string_test() {
	test_string_return("string-concat.snp", "this is a string", "Chained string concatenation");
	test_string_return("string.snp", "snap = good", "String cocatenation in blocks");
	std::cout << "[String tests passed]\n";
}

int main() {
	expr_tests();
	stmt_tests();
	fn_tests();
	table_test();
	string_test();
	return 0;
}