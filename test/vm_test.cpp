#include "assert.hpp"
#include "test_utils.hpp"
#include "value.hpp"
#include "vm.hpp"
#include <fstream>
#include <memory>
#include <sstream>
#include <stdlib.h>

using namespace vyse;

void assert_val_eq(Value expected, Value actual, const char* message = "Test failed! ") {
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
	vm.load_stdlib();
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

	if (!VYSE_IS_STRING(vm.return_value)) {
		std::cout << "[ Failed ]" << message << "Expected string but got ";
		print_value(vm.return_value);
		std::cout << "\n";
		abort();
	}

	if (std::memcmp(VYSE_AS_CSTRING(vm.return_value), expected, strlen(expected)) != 0) {
		std::cout << message << "[ STRINGS NOT EQUAL ] \n";
		std::cout << "Expected: '" << expected << "'.\n";
		std::cout << "Got: '" << VYSE_AS_CSTRING(vm.return_value) << "'\n";
		abort();
	}
}

static void expr_tests() {
	test_return("return 5 / 2", VYSE_NUM(2.5));
	test_return("return 5 % 2", VYSE_NUM(1.0));
	test_return("return 5 + 2", VYSE_NUM(7.0));
	test_return("return 5 - 2", VYSE_NUM(3.0));
	test_return("return 3 * 5", VYSE_NUM(15.0));

	test_return("return 3 < 5", VYSE_BOOL(true));
	test_return("return 3 > 5", VYSE_BOOL(false));
	test_return("return 3 <= 3", VYSE_BOOL(true));
	test_return("return 4 >= 4", VYSE_BOOL(true));

	test_return("return 10 - 5 - 2", VYSE_NUM(3.0));
	test_return("return 10 && 5 && 2", VYSE_NUM(2));
	test_return("return 10 || 5 || 2", VYSE_NUM(10));
	test_return("return 10 - 5 - 2", VYSE_NUM(3.0));

	test_return(R"(
		let a = 1
		let b = 3
		a = 5
		b = 4 
		return a + b
	)",
							VYSE_NUM(9), "top level local variables");

	test_return("return 9 & 7", VYSE_NUM(1));
	test_return("return 4 | 9", VYSE_NUM(13));

	// test precedence

	test_return("return 5 > 2 && 3 > -10", VYSE_BOOL(true));

	// test string equality
	test_return("return 'abc' == 'abc'", VYSE_BOOL(true));
	test_return(R"(
		const a = 'abcde'
		const b = "abcde"
		const c = 'aa'
		const d = 'bb'

		return a == b and c..d == 'aabb'
	)",
							VYSE_BOOL(true));

	test_file("compound-assign.vy", VYSE_NUM(8), "Compound assignment operators");

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
							VYSE_NUM(5), "If statement without else branch");

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
							VYSE_NUM(7), "If statement with else-if branch");

	test_file("var.vy", VYSE_NUM(13), "block scoped declarations.");

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
							VYSE_NUM(30), "chained calls with parameters");

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
							VYSE_NUM(6), "Closures and chained function calls");

	test_return(R"(
		fn fib(n) {
			if (n <= 1) return 1
			return fib(n - 1) + fib(n - 2)
		}
		return fib(10)
	)",
							VYSE_NUM(89), "Recursive fiboacci");

	test_return(R"(
		fn make_adder(x) {
			return fn(y) {
				return x + y
			}
		}

		const add10 = make_adder(10)
		return add10(-10)
	)",
							VYSE_NUM(0), "make_adder closure");

	test_file("llnode-cl.vy", VYSE_NUM(20), "Linked list closure test");
	test_file("call.vy", VYSE_NUM(20), "Call stack");
}

void table_test() {
	test_return(R"(
	const T = 	{
		a: 1, b : 2
	} 
	return T.a + T.b
	)",
							VYSE_NUM(3), " Indexing tables with '.' operator");

	test_return(R"(
		fn Node(a, b) {
			return  { data: a, next: b }
		}
		const head = Node(10, Node(20))
		return head.next.data
	)",
							VYSE_NUM(20), "Linked lists as tables test");

	// setting table field names.
	test_return(R"(
		const t = {
			k : 7
		}
		let a = t.k
		t.k = 3
		return t.k +  a
	)",
							VYSE_NUM(10), "setting table field names");

	test_file("table-1.vy", VYSE_NUM(3), "returning tables from a closure");
	test_file("table-2.vy", VYSE_NUM(6), "Accessing table fields");
	test_file("table-3.vy", VYSE_NUM(11), "Accessing table fields");
	test_file("table-4.vy", VYSE_NUM(10), "Computed member assignment and access");
	test_file("table-5.vy", VYSE_NUM(10),
						"Computed member access and dot member access are equivalent for string keys");
	test_file("table-6.vy", VYSE_NUM(25), "Compound assignment to computed members");
	test_file("table-7.vy", VYSE_NUM(10), "Syntactic sugar for table methods");
	test_file("table-8.vy", VYSE_NUM(1), "Empty tables.");
	test_file("table-9.vy", VYSE_NUM(30), "Chained dot and subscript operators.");
	test_file("keys.vy", VYSE_NUM(6), "Subscript operator in table key with interned strings");
	test_file("point.vy", VYSE_NUM(50), "Constructor like functions. (point.vy)");
	test_file("method-1.vy", VYSE_NUM(3), "Method call syntax.");
	test_file("method-2.vy", VYSE_NUM(5), "Chained method calls.");
	test_file("suffix-expr.vy", VYSE_NUM(123), "Chained suffix expressions.");
	test_file("inherit.vy", VYSE_NUM(21), "Prototypical inheritance with setmeta builtin.");
	test_file("self.vy", VYSE_NUM(6), "Prototypical inheritance with setmeta builtin.");
	test_file("global.vy", VYSE_NUM(6), "Global variables.");

	std::cout << "[Table tests passed]\n";
}

void global_test() {
	VM vm;
	vm.set_global("foo", VYSE_NUM(42));
	assert_val_eq(vm.get_global("foo"), VYSE_NUM(42), "Global variables.");
}

void string_test() {
	test_string_return("string-concat.vy", "this is a string", "Chained string concatenation");
	test_string_return("string.vy", "snap = good", "String cocatenation in blocks");
	std::cout << "[String tests passed]\n";
}

void loop_test() {
	test_file("loop-while.vy", VYSE_NUM(45), "While loops (sum)");
	test_file("nested-while.vy", VYSE_NUM(165), "Nested While loops");
	test_file("while-break.vy", VYSE_NUM(25), "breaks in loops");
	test_file("while-break-nest.vy", VYSE_NUM(2660), "breaks in loops");
	test_file("continue.vy", VYSE_NUM(55), "continue statements in loops.");
	test_file("while-closure.vy", VYSE_NUM(55), "closure inside a while loop.");
}

int main() {
	expr_tests();
	stmt_tests();
	fn_tests();
	table_test();
	global_test();
	string_test();
	loop_test();
	return 0;
}