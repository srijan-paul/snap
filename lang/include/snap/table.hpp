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
	/// to calculate mod.
	static constexpr std::size_t DefaultCapacity = 8;
	static constexpr u8 GrowthFactor = 2;
	static constexpr float LoadFactor = 0.85;

	/// @return The value assosciated with `key`.
	Value get(Value key) const;

	/// @brief Removes a key from the Hashtable.
	Value remove(Value key);

	/// @brief set table[key] = value.
	/// @return true if a new entry was created, otherwise
	/// return false.
	bool set(Value key, Value value);

	/// @brief Takes a string C string on the heap. If
	/// a snap::String exists with the same characters,
	/// then returns a pointer to it, otherwise returns
	/// `nullptr`.
	String* find_string(const char* chars, std::size_t length);

	Value& operator[](std::size_t index);

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
		u32 probe_distance;
	};

  private:
	Entry* m_entries = new Entry[DefaultCapacity];
	std::size_t m_num_entries = 0;
	std::size_t m_cap = DefaultCapacity;

	/// @brief the metatable for this table.
	/// If a property is not found in this table
	/// then a lookup is done on the meta table.
	Table* m_meta_table = nullptr;

	std::size_t hash_value(Value value) const;
	std::size_t hash_object(Obj* object) const;

	/// @brief If the hashtable is [LoadFactor]th full
	/// then grows the entries buffer.
	void ensure_capacity();

	template <typename Th, typename Rt>
	Rt& search_entry(const Th* this_, const Value& key, std::size_t hash) {
		std::size_t mask = this_->m_cap - 1;
		std::size_t index = hash & mask;

		while (true) {
			Entry& entry = this_->m_entries[index];
			if (entry.key == key or entry.hash == 0) return entry;
			index = (index + 1) & mask;
		}

		return this_->m_entries[0];
	}

	/// @brief Finds an entry in the hastable
	/// Where `key` should be mapped to.
	Entry& find(Value key);
	const Entry& find(Value value) const;
};

bool operator==(const Table::Entry& a, const Table::Entry& b);

} // namespace snap