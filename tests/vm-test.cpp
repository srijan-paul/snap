#include "assert.hpp"
#include "util/test_utils.hpp"
#include "value.hpp"
#include "vm.hpp"
#include <fstream>
#include <memory>
#include <stdlib.h>

using namespace vy;

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

	test_return("return 9 & 7", VYSE_NUM(1));
	test_return("return 9 ^ 9", VYSE_NUM(0));
	test_return("return 13 | 42 & 24 ^ 31 | 12", VYSE_NUM(31));
	test_return("return 4 | 9", VYSE_NUM(13));
	test_return("return 4 << 9 >> 5", VYSE_NUM(64));
	test_return("return 4 >> 5 >> 7 | 6", VYSE_NUM(6));
	test_return("return 4 >> 1 | 2 | 3 << 5", VYSE_NUM(98));
	test_return("return ~68 * ~~12", VYSE_NUM(-828));

	test_return("return 5 ** 2", VYSE_NUM(25));
	test_return("return 3 ** 2 ** 3", VYSE_NUM(729));

	// test precedence
	test_return("return 5 > 2 && 3 > -10", VYSE_BOOL(true));
	test_return("return -#'xxx'", VYSE_NUM(-3));
	test_return("return #'xxx'", VYSE_NUM(3));
	test_return("return - - - - #'xxxxx'", VYSE_NUM(5));
	test_return("return !!{}", VYSE_BOOL(true));

	// test equality between values
	test_return("return 'abc' == 'abc'", VYSE_BOOL(true));
	test_return("return 'abc' == 'a'..'bc'", VYSE_BOOL(true), "concatenating strings");
	test_return("return 'abc' == ''..'abc'..''", VYSE_BOOL(true),
				"appending/prepending empty strings has no effect.");
	test_return("return nil == nil", VYSE_BOOL(true), "nil == nil");
	test_return("return nil == false", VYSE_BOOL(false), "nil != false");
	test_return("return nil == true", VYSE_BOOL(false), "nil != true");
	test_return("return 1 == true", VYSE_BOOL(false), "1 != true");
	test_return("return 123 == '123'", VYSE_BOOL(false), "123 != '123'");
	test_return("return '0' == 48", VYSE_BOOL(false),
				"chars and strings aren't compared using ASCII values");
	test_return("return 0 == false", VYSE_BOOL(false), "0 != false");
	test_return("return {} == {}", VYSE_BOOL(false), "empty tables are not equal");

	test_file("expr/compound-assign.vy", VYSE_NUM(8), "Compound assignment operators");
	test_file("expr/len.vy", VYSE_NUM(5), "Number of entries in a table using '#' operator.");
	test_file("expr/len-str.vy", VYSE_NUM(9), "string length using '#' operator.");
	test_file("statements/locals.vy", VYSE_NUM(9), "top level local variables");
	test_file("expr/concat.vy", VYSE_BOOL(true));

	std::cout << "[Expression tests passed]\n";
}

static void stmt_tests() {
	test_file("statements/var.vy", VYSE_NUM(13), "block scoped declarations.");
	test_file("if/if.vy", VYSE_NUM(5), "If statement without else branch");
	test_file("if/else-if.vy", VYSE_NUM(7), "If statement with an else-if branch");
	test_file("statements/global.vy", VYSE_NUM(6), "Global variables.");

	std::cout << "Statement tests passed\n";
}

void fn_tests() {
	test_file("closures/adder-2.vy", VYSE_NUM(30), "chained calls with parameters");
	test_file("closures/nested-closures.vy", VYSE_NUM(6), "Closures and chained function calls");
	test_file("closures/fib-rec.vy", VYSE_NUM(89), "Recursive fiboacci");
	test_file("closures/adder.vy", VYSE_NUM(0), "make_adder closure");
	test_file("closures/llnode-cl.vy", VYSE_NUM(20), "Linked list closure test");
	test_file("closures/call.vy", VYSE_NUM(20), "Call stack");
	test_file("closures/gc-closure.vy", VYSE_NUM(40), "Closures with stress GC");
}

void table_test() {
	test_return(R"(
	const T = 	{
		a: 1, b : 2
	} 
	return T.a + T.b
	)",
				VYSE_NUM(3), " Indexing tables with '.' operator");

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

	test_file("tables/table-1.vy", VYSE_NUM(3), "returning tables from a closure");
	test_file("tables/table-2.vy", VYSE_NUM(6), "Accessing table fields");
	test_file("tables/table-3.vy", VYSE_NUM(11), "Accessing table fields");
	test_file("tables/table-4.vy", VYSE_NUM(10), "Computed member assignment and access");
	test_file("tables/table-5.vy", VYSE_NUM(10),
			  "tables/Computed member access and dot member access are equivalent for string keys");
	test_file("tables/table-6.vy", VYSE_NUM(25), "Compound assignment to computed members");
	test_file("tables/table-7.vy", VYSE_NUM(10), "Syntactic sugar for table methods");
	test_file("tables/table-8.vy", VYSE_NUM(1), "Empty tables.");
	test_file("tables/table-9.vy", VYSE_NUM(30), "Chained dot and subscript operators.");
	test_file("tables/keys.vy", VYSE_NUM(6),
			  "Subscript operator in table key with interned strings");
	test_file("tables/point.vy", VYSE_NUM(50), "Constructor like functions. (point.vy)");
	test_file("tables/method-1.vy", VYSE_NUM(3), "Method call syntax.");
	test_file("tables/method-2.vy", VYSE_NUM(5), "Chained method calls.");
	test_file("tables/suffix-expr.vy", VYSE_NUM(123), "Chained suffix expressions.");
	test_file("tables/inherit.vy", VYSE_NUM(21), "Prototypical inheritance with setmeta builtin.");
	test_file("tables/self.vy", VYSE_NUM(6), "Prototypical inheritance with setmeta builtin.");
	test_file("tables/link-list.vy", VYSE_NUM(20), "Linked lists as tables test");

	std::cout << "[Table tests passed]\n";
}

void global_test() {
	VM vm;
	vm.set_global("foo", VYSE_NUM(42));
	assert_val_eq(vm.get_global("foo"), VYSE_NUM(42), "Global variables.");
}

void string_test() {
	test_string_return("strings/string-concat.vy", "this is a string",
					   "Chained string concatenation");
	test_string_return("strings/string.vy", "snap = good", "String cocatenation in blocks");
	test_string_return("strings/gc-strcat.vy", "xyz", "String cocatenation and GC");
	std::cout << "[String tests passed]\n";
}

void loop_test() {
	// while loops
	test_file("loop/while-loop.vy", VYSE_NUM(45), "While loops (sum)");
	test_file("loop/nested-while.vy", VYSE_NUM(165), "Nested While loops");
	test_file("loop/while-break.vy", VYSE_NUM(25), "breaks in loops");
	test_file("loop/while-break-nest.vy", VYSE_NUM(2660), "breaks in loops");
	test_file("loop/continue.vy", VYSE_NUM(55), "continue statements in loops");
	test_file("loop/closure-in-while.vy", VYSE_NUM(55), "closure inside a while loop");
	test_file("loop/fib-loop.vy", VYSE_NUM(89), "Fibonacci implementation using loop");

	// for-loops
	test_file("loop/for/loop-for.vy", VYSE_NUM(45), "for-loop only with counter and step");
	test_file("loop/for/loop-locvar.vy", VYSE_NUM(12379), "for-loop with local vars");
	test_file("loop/for/loop-step.vy", VYSE_NUM(20), "for-loop with explicit step");
	test_file("loop/for/for-rev-1.vy", VYSE_NUM(55), "reverse for loop");
	test_file("loop/for/for-rev.vy", VYSE_NUM(166), "for-loop that counts downwards");
	test_file("loop/for/continue.vy", VYSE_NUM(25), "continue in for-loop");
	test_file("loop/for/break.vy", VYSE_NUM(28), "break in for-loop");
	test_file("loop/for/for-mut-counter.vy", VYSE_NUM(45),
			  "for-loop that tries to change the counter inside loop body");
	test_file("loop/for/nest.vy", VYSE_NUM(870), "nested for loops");
	test_file("loop/for/in-closure.vy", VYSE_NUM(110), "for-loop inside closure.");
}

void multiple_runs_test() {
	VM vm;
	vm.load_stdlib();
	const char* fail_message = "Cannot call VM::run multiple times.";
	vm.runcode("a = (/x -> x * 2)(2)");
	auto res = vm.runcode("assert(a == 4)");
	ASSERT(res == ExitCode::Success, fail_message);
}

int main() {
	expr_tests();
	stmt_tests();
	fn_tests();
	table_test();
	global_test();
	string_test();
	loop_test();
	multiple_runs_test();
	return 0;
}
