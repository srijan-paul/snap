#pragma once
#include "string.hpp"
#include "value.hpp"
#include <cmath>
#include <std/base.hpp>

namespace vyse {

// This is the core table data structure
// in vyse. I decided to go with a hashtable
// implementation that uses Robinhood hashing
// and linear probing.
class Table final : public Obj {
	friend GC;
	friend Value stdlib::setproto(VM&, int);
	friend Value stdlib::getproto(VM&, int);

public:
	explicit Table() noexcept : Obj{ObjType::table} {};
	~Table();

	/// IMPORTANT: `DefaultCapacity` must always be a
	/// power of two, since we are using the `&` trick
	/// to calculate fast mod.
	static constexpr size_t DefaultCapacity = 16;
	static constexpr u8 GrowthFactor = 2;
	static constexpr float LoadFactor = 0.85;

	/// @return The value assosciated with `key`.
	Value get(Value key) const;

	/// @brief Removes a key from the Hashtable.
	/// and returns true if [key] really did exist
	/// in the table before deletion.
	bool remove(Value key);

	/// @brief Set table[key] = value.
	/// @return true if a new entry was created, otherwise
	/// return false.
	bool set(Value key, Value value);

	/// @return The number of key-value pairs that
	/// are active in this table.
	size_t length() const;

	/// @brief Takes a string C string on the heap. checks if
	/// a vyse::String exists with the same characters.
	/// @return A pointer to the string object, if found
	/// inside the table, else nullptr.
	String* find_string(const char* chars, size_t length, size_t hash) const;

	/// Returns the total number of alive entries in
	/// this hashtable. values that have been set to nil
	/// don't count.
	virtual size_t size() const override;

	/// An Entry represents a key-value pair
	/// in the hashtable, both the key and the
	/// value are of type `vyse::Value`.
	/// An empty slot is represented by an Entry
	/// whose key is Nil and a tombstone is
	/// represented by an entry whose key's
	/// type tag is VT::Undefined.
	struct Entry {
		Value key;
		Value value;
		size_t hash = 0;
		// Distance of this entry from
		// it's desired slot in the hashtable.
		// The desired slot is the very first
		// slot we land on. For a value X,
		// it's desired slot is `hash(X) % m_cap`
		size_t probe_distance = 0;
	};

private:
	Entry* m_entries = new Entry[DefaultCapacity];
	/// @brief Total number of entries.
	/// This includes all tombstones (values that have been
	/// removed from the table).
	size_t m_num_entries = 0;
	/// @brief The total number of tombstones in the table.
	/// A tombstone is an entry that was inserted at some
	/// point but was then removed by calling `Table::remove`.
	size_t m_num_tombstones = 0;
	size_t m_cap = DefaultCapacity;

	/// @brief The prototype for this table.
	/// If a property is not found in this table
	/// then a lookup is done on the prototype.
	Table* m_proto_table = nullptr;

	size_t hash_value(Value value) const;
	size_t hash_object(Obj* object) const;

	/// @brief If the hashtable is [LoadFactor]th full
	/// then grows the entries buffer.
	void ensure_capacity();

	/// @brief Using a key and it's hash, returns the slot in the
	/// entries array where the key should be inserted.
	template <typename Th, typename Rt>
	// The reason this is a template and not a member function is
	// because we might need const and non-const overloads, and repeating
	// code for that isn't the coolest thing to do.
	Rt& search_entry(const Th* this_, const Value& key, size_t hash) {
		size_t mask = this_->m_cap - 1;
		size_t index = hash & mask;

		// We store the value of the first tombstone.
		// If we are unable to find any entry with the
		// same key as the one we're looking for then
		// we return the first tombstone instead.
		Entry* first_tombstone = nullptr;
		while (true) {
			Entry& entry = this_->m_entries[index];
			// If we hit an entry with the same key as the current location,
			// then we return that. If we hit a tombstone, and it's the first tombstone
			// we came across, then store that slot.
			// If no slot with that key is found, then we must be looking for a slot to insert a new
			// element into the hash table, so we return the tombstone as a slot for insertion.
			if (VYSE_IS_UNDEFINED(entry.key) and first_tombstone == nullptr) {
				first_tombstone = &entry;
			} else if ((entry.hash == hash and entry.key == key)) {
				return entry;
			} else if (VYSE_IS_NIL(entry.key)) {
				return first_tombstone ? *first_tombstone : entry;
			}

			index = (index + 1) & mask;
		}
	}

	virtual void trace(GC& gc) override;

	/// @brief Deletes all the string keys that
	/// aren't marked as 'alive' by the previous GC mark phase.
	void delete_white_string_keys();
};

bool operator==(const Table::Entry& a, const Table::Entry& b);

} // namespace vyse 