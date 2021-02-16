#include "assert.hpp"
#include "string.hpp"
#include "test_utils.hpp"
#include "value.hpp"
#include <table.hpp>

#define NUM		 SNAP_NUM_VAL
#define NIL		 SNAP_NIL_VAL
#define STR(...) (new snap::String(__VA_ARGS__))

using string_ptr = std::unique_ptr<snap::String>;

bool table_has_cstring(snap::Table& t, const char* cs) {
	const size_t len = strlen(cs);
	snap::String* s = t.find_string(cs, len);
	if (s == nullptr) return false;
	return true;
}

void run_test() {
	snap::Table table;
	EXPECT(table.get(NUM(1)) == NIL, "Non-existent keys return nil.");
	table.set(NUM(1), NUM(2));
	EXPECT(table.get(NUM(1)) == NUM(2), "number-number key values.");

	table.remove(NUM(1));
	EXPECT(table.get(NUM(1)) == NIL, "Removing keys sets them to nil.");

	string_ptr s(STR("hello", 5));
	table.set(s.get(), NUM(1));
	EXPECT(table_has_cstring(table, "hello"), "Table::find_string works as expected.");
}

void resize_test() {
	snap::Table t;
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
	snap::Table t;
	for (int i = 0; i < 1000; ++i) t.set(NUM(i), NUM(i * 3));
	for (int i = 0; i < 1000; ++i) EXPECT(t.get(NUM(i)) == NUM(i * 3), "Table::get.");

	for (int i = 900; i < 1000; ++i) t.remove(NUM(i));
	for (int i = 0; i < 900; ++i)
		EXPECT(t.get(NUM(i)) == NUM(i * 3),
			   "'Table::get' after removal of 10% of the values. (Failed at: " << i << ")");
	for (int i = 900; i < 1000; ++i)
		EXPECT(SNAP_IS_NIL(t.get(NUM(i))), "Deleted keys when queried for, return nil @" << i);

	for (int i = 900; i < 1000; ++i) t.set(NUM(i), NUM(i * 4));
	for (int i = 900; i < 1000; ++i)
		EXPECT(t.get(NUM(i)) == NUM(i * 4),
			   "Table::get -> Quering for keys works on deleting, and then reinserting. (Failure @"
				   << i << ")");
}

void strkey_test() {
	snap::Table t;
	const char* sk = "this is a random key.";
	const char* sv = "this is a random value.";

	string_ptr key(STR(sk, strlen(sk)));
	string_ptr value(STR(sv, strlen(sv)));

	t.set(SNAP_OBJECT_VAL(key.get()), SNAP_OBJECT_VAL(value.get()));
	EXPECT(t.get(SNAP_OBJECT_VAL(key.get())) == SNAP_OBJECT_VAL(value.get()),
		   "Key-value pairs where both key and value are strings");
}

int main() {
	run_test();
	resize_test();
	removal_test();
	strkey_test();

	return 0;
}