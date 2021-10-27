#include "assert.hpp"
#include "string.hpp"
#include "util/test_utils.hpp"
#include "value.hpp"
#include <table.hpp>

#define NUM		 VYSE_NUM
#define NIL		 VYSE_NIL
#define STR(...) (new vy::String(__VA_ARGS__))
#define BOOL(t)	 VYSE_BOOL(t)

using unique_str_ptr = std::unique_ptr<vy::String>;
using shared_str_ptr = std::shared_ptr<vy::String>;

bool table_has_cstring(vy::Table& t, const char* cs) {
	const size_t len = strlen(cs);
	vy::String* s = t.find_string(cs, len, vy::hash_cstring(cs, len));
	if (s == nullptr) return false;
	return true;
}

void run_test() {
	vy::Table table;
	EXPECT(table.get(NUM(1)) == NIL, "Non-existent keys return nil.");
	table.set(NUM(1), NUM(2));
	EXPECT(table.get(NUM(1)) == NUM(2), "number-number key values.");

	table.remove(NUM(1));
	EXPECT(table.get(NUM(1)) == NIL, "Removing keys sets them to nil.");

	unique_str_ptr s(STR("hello", 5));
	table.set(VYSE_OBJECT(s.get()), NUM(1));
	EXPECT(table_has_cstring(table, "hello"), "Table::find_string works as expected.");
}

void resize_test() {
	vy::Table t;
	for (int i = 0; i < 1000; ++i) {
		t.set(NUM(i), NUM(i * 2));
	}

	for (int i = 0; i < 1000; ++i) {
		EXPECT(t.get(NUM(i)) == NUM(i * 2), "Works after growing in size. @" << i);
	}
}

/// 1. Insert 1000 key-value pairs into the hashtable, then query and check if that worked.
/// 2. Remove 10% of the inserted keys (900-1000), and then check if querying for them returns nil.
///    Also check if the querying for the other 90%  works as well.
/// 3. Reinsert the keys 900-1000 and test the queries again.
void removal_test() {
	vy::Table t;
	for (int i = 0; i < 1000; ++i) t.set(NUM(i), NUM(i * 3));
	for (int i = 0; i < 1000; ++i) EXPECT(t.get(NUM(i)) == NUM(i * 3), "Table::get.");

	for (int i = 900; i < 1000; ++i) t.remove(NUM(i));
	for (int i = 0; i < 900; ++i)
		EXPECT(t.get(NUM(i)) == NUM(i * 3),
			   "'Table::get' after removal of 10% of the values. (Failed at: " << i << ")");
	for (int i = 900; i < 1000; ++i)
		EXPECT(VYSE_IS_NIL(t.get(NUM(i))), "Deleted keys when queried for, return nil @" << i);

	for (int i = 900; i < 1000; ++i) t.set(NUM(i), NUM(i * 4));
	for (int i = 900; i < 1000; ++i)
		EXPECT(t.get(NUM(i)) == NUM(i * 4),
			   "Table::get -> Quering for keys works on deleting, and then reinserting. (Failure @"
				   << i << ")");
}

void strkey_test() {
	vy::Table t;
	const char* sk = "this is a random key.";
	const char* sv = "this is a random value.";

	unique_str_ptr key(STR(sk, strlen(sk)));
	unique_str_ptr value(STR(sv, strlen(sv)));

	t.set(VYSE_OBJECT(key.get()), VYSE_OBJECT(value.get()));
	EXPECT(t.get(VYSE_OBJECT(key.get())) == VYSE_OBJECT(value.get()),
		   "Key-value pairs where both key and value are strings");

	unique_str_ptr key2(STR(sk, strlen(sk)));
	EXPECT(t.get(VYSE_OBJECT(key2.get())) != VYSE_OBJECT(value.get()),
		   "Strings don't have reference equality when not interned.");
}

vy::String* make_interned_string(vy::Table& intern_table, const char* cs, int len) {
	vy::String* interned = intern_table.find_string(cs, len, vy::hash_cstring(cs, len));
	if (interned == nullptr) {
		auto* s = new vy::String(cs, len);
		intern_table.set(VYSE_OBJECT(s), BOOL(true));
		return s;
	}
	return interned;
}

void intern_test() {
	vy::Table t;
	const char* s1 = "a short string.";
	vy::String* s = make_interned_string(t, s1, strlen(s1));
	EXPECT(t.length() == 1,
		   "Table::length() - is 1 when there is one string entry in the Intern table. (got: "
			   << t.length() << ")");

	int s1len = strlen(s1);
	EXPECT(t.find_string(s1, strlen(s1), vy::hash_cstring(s1, s1len)) != nullptr,
		   "Table::find_string test.");

	vy::String* s_ = make_interned_string(t, s1, strlen(s1));
	EXPECT(s_ == s, "String comparison can be done using pointers when interned (got "
						<< std::hex << s_ << " != " << s << ").");

	// If this test passes, then s and s_ refer to the same string in memory,
	// in which case, deleting via any one pointer is sufficient, if the test
	// doesn't pass then we abort anyway.
	delete s;
}

int main() {
	run_test();
	resize_test();
	removal_test();
	strkey_test();
	intern_test();

	std::cout << "[All Table Tests Passed]\n";

	return 0;
}