#pragma once
#include <cmath>
#include <string.hpp>
#include <unordered_map>
#include <value.hpp>

namespace snap {

// This is the core table data structure
// in snap. I decided to go with a hashtable
// implementation that uses Robinhood hashing
// and linear probing.
class Table final : public Obj {
  public:
	explicit Table() noexcept : Obj{ObjType::table} {};
	~Table();

	/// IMPORTANT: `Default` Capacity must always be a
	/// power of two, since we are using the `&` trick
	/// to calculatefast mod.
	static constexpr std::size_t DefaultCapacity = 16;
	static constexpr u8 GrowthFactor = 2;
	static constexpr float LoadFactor = 0.85;

	/// @return The value assosciated with `key`.
	Value get(Value key) const;

	/// @brief Removes a key from the Hashtable.
	/// and returns true if [key] really did exist
	/// in the table before deletion.
	bool remove(Value key);

	/// @brief set table[key] = value.
	/// @return true if a new entry was created, otherwise
	/// return false.
	bool set(Value key, Value value);

	/// @brief Takes a string C string on the heap. If
	/// a snap::String exists with the same characters,
	/// then returns a pointer to it, otherwise returns
	/// `nullptr`.
	String* find_string(const char* chars, std::size_t length);

	/// An Entry represents a key-value pair
	/// in the hashtable, both the key and the
	/// value are of type `snap::Value`.
	/// An empty slot is represented by an Entry
	/// whose key is Nil and a tombstone is
	/// represented by an entry whose key's
	/// type tag is VT::Empty.
	struct Entry {
		Value key;
		Value value;
		u64 hash = 0;
		// Distance of this entry from
		// it's desired slot in the hashtable.
		// The desired slot is the very first
		// slot we land on. For a value X,
		// it's desired slot is `hash(X) % m_cap`
		u32 probe_distance = 0;
	};

  private:
	Entry* m_entries = new Entry[DefaultCapacity];
	/// @brief total number of entries.
	/// This includes all tombstones (values that have been
	/// removed from the table).
	u64 m_num_entries = 0;
	u64 m_cap = DefaultCapacity;

	/// @brief the metatable for this table.
	/// If a property is not found in this table
	/// then a lookup is done on the meta table.
	Table* m_meta_table = nullptr;

	u64 hash_value(Value value) const;
	u64 hash_object(Obj* object) const;

	/// @brief If the hashtable is [LoadFactor]th full
	/// then grows the entries buffer.
	void ensure_capacity();

	/// @brief Using a key and it's hash, returns the slot in the
	/// entries array where the key should be inserted.
	template <typename Th, typename Rt>
	Rt& search_entry(const Th* this_, const Value& key, std::size_t hash) {
		std::size_t mask = this_->m_cap - 1;
		std::size_t index = hash & mask;

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
			if (SNAP_IS_EMPTY(entry.key) and first_tombstone == nullptr) {
				first_tombstone = &entry;
			} else if ((entry.hash == hash and entry.key == key)) {
				return entry;
			} else if (SNAP_IS_NIL(entry.key)) {
				return first_tombstone ? *first_tombstone : entry;
			}

			index = (index + 1) & mask;
		}
	}
};

bool operator==(const Table::Entry& a, const Table::Entry& b);

} // namespace snap