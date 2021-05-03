#include "common.hpp"
#include "value.hpp"
#include <gc.hpp>
#include <table.hpp>
#include <upvalue.hpp>

namespace vyse {

using VT = ValueType;
using OT = ObjType;

#define TABLE_GET_SLOT(k, h)			 search_entry<Table, Entry>(this, k, h)
#define TABLE_GET_SLOT_CONST(k, h) search_entry<const Table, const Entry>(this, k, h)
#define TABLE_PLACE_TOMBSTONE(e)                                                                   \
	(VYSE_SET_TT(e.key, VT::Undefined), e.value = VYSE_NIL_VAL, ++m_num_tombstones)

// check if an entry is unoccupied.
#define IS_ENTRY_FREE(e) (VYSE_IS_NIL(e.key))
#define IS_ENTRY_DEAD(e) (VYSE_IS_UNDEFINED(e.key))
#define HASH_OBJ(o)			 ((size_t)(o)&UINT64_MAX)

Table::~Table() {
	delete[] m_entries;
}

void Table::ensure_capacity() {
	if (m_num_entries < m_cap * LoadFactor) return;
	size_t old_cap = m_cap;
	m_cap *= GrowthFactor;
	Entry* old_entries = m_entries;
	m_entries = new Entry[m_cap];

	for (size_t i = 0; i < old_cap; ++i) {
		Entry& entry = old_entries[i];
		// We don't re-insert tombstones or entries that were
		// never occupied in the first place.
		if (IS_ENTRY_FREE(entry) or IS_ENTRY_DEAD(entry)) continue;
		Entry& new_entry = TABLE_GET_SLOT(entry.key, entry.hash);
		new_entry = std::move(entry);
	}

	// We only copied over the actual entries, and
	// left behind all the tombstones. So we change
	// these two member variables to reflect that.
	m_num_entries -= m_num_tombstones;
	m_num_tombstones = 0;

	delete[] old_entries;
}

Value Table::get(Value key) const {
	if (VYSE_IS_NIL(key)) return VYSE_NIL_VAL;

	size_t mask = m_cap - 1;
	size_t hash = hash_value(key);
	size_t index = hash & mask;

	while (true) {
		Entry& entry = m_entries[index];
		if (entry.hash == hash and entry.key == key) return entry.value;
		if (IS_ENTRY_FREE(entry)) break;
		index = (index + 1) & mask;
	}

	return m_proto_table == nullptr ? VYSE_NIL_VAL : m_proto_table->get(key);
}

/// TODO: handle the case for [set] where the [key] is a member of it's
/// prototype.
bool Table::set(Value key, Value value) {
	VYSE_ASSERT(!VYSE_IS_NIL(key), "Table key is nil.");

	// If the value is nil, then the key is
	// simply removed with a tombstone in the
	// table.
	if (VYSE_IS_NIL(value)) return remove(key);

	ensure_capacity();
	size_t hash = hash_value(key);
	size_t mask = m_cap - 1;

	size_t index = hash & mask;
	// The probe distance that we have covered so far.
	// Initially we are at our "desired" slot, where
	// our entry would ideally sit. So the probe distance is
	// 0.
	u32 dist = 0;

	while (true) {
		Entry& entry = m_entries[index];

		// If we found an unitialized spot or a
		// tombstone in the entries buffer, use that
		// slot for insertion.
		const bool is_free = IS_ENTRY_FREE(entry);
		const bool is_tombstone = IS_ENTRY_DEAD(entry);

		if (is_free or is_tombstone) {
			entry.key = std::move(key);
			entry.value = std::move(value);
			entry.probe_distance = dist;
			entry.hash = hash;
			// Only increment number of entries
			// if the slot was free.
			if (is_free) ++m_num_entries;
			// Even if it was a tombstone,
			// we still inserted a new key
			// into the hashtable, so we
			// return true.
			return true;
		}

		// if we found an intialized list that has the
		// same key as the current key we're trying to set,
		// then change the value in that entry.
		if (entry.key == key) {
			entry.value = std::move(value);
			entry.probe_distance = dist;
			return false;
		}

		// if we found an entry that isn't what
		// we're looking for, but has a lower
		// probe count, then we swap the entries
		// and keep going.
		if (entry.probe_distance < dist) {
			std::swap(hash, entry.hash);
			std::swap(key, entry.key);
			std::swap(value, entry.value);
			dist = entry.probe_distance;
		}

		index = (index + 1) & mask;
		++dist;
	}

	return true;
}

bool Table::remove(Value key) {
	if (m_num_entries == 0) return false;

	// Find the slot where this key would go.
	// If the key exists in the table, then the
	// corresponding entry containing the key-value
	// pair is returned, otherwise we get the slot
	// where the key would have been placed, if it
	// did exist in the table.
	Entry& entry = TABLE_GET_SLOT(key, hash_value(key));

	// This entry was never occupied by a key-value
	// pair in the first place, then no deletion is
	// neccessary;
	if (IS_ENTRY_FREE(entry) or IS_ENTRY_DEAD(entry)) return false;

	TABLE_PLACE_TOMBSTONE(entry);
	return true;
}

size_t Table::length() const {
	return m_num_entries - m_num_tombstones;
}

String* Table::find_string(const char* chars, size_t length, size_t hash) const {
	VYSE_ASSERT(chars != nullptr, "key string is null.");
	VYSE_ASSERT(hash == hash_cstring(chars, length), "Incorrect cstring hash.");

	size_t mask = m_cap - 1;
	size_t index = hash & mask;

	while (true) {
		Entry& entry = m_entries[index];

		if (entry.hash == hash) {
			Value& k = entry.key;
			if (VYSE_IS_STRING(k)) {
				String* s = VYSE_AS_STRING(k);
				if (s->len() == length and std::memcmp(s->c_str(), chars, length) == 0) return s;
			}
		}

		// we have hit an empty slot, meaning there
		// is no such string in the hashtable.
		if (IS_ENTRY_FREE(entry)) return nullptr;

		index = (index + 1) & mask;
	}

	return nullptr;
}

size_t Table::hash_value(Value key) const {
	VYSE_ASSERT(!VYSE_IS_NIL(key), "Attempt to hash a nil key.");
	switch (VYSE_GET_TT(key)) {
	case VT::Bool: return VYSE_AS_BOOL(key) ? 7 : 15;
	case VT::Number: return size_t(VYSE_AS_NUM(key)); // TODO: use a proper numeric hash
	case VT::Object: return hash_object(VYSE_AS_OBJECT(key));
	default: VYSE_UNREACHABLE(); 
	}
}

size_t Table::hash_object(Obj* object) const {
	switch (object->tag) {
	case OT::string: return static_cast<String*>(object)->hash();
	case OT::upvalue: return hash_value(*static_cast<Upvalue*>(object)->m_value);
	default: return HASH_OBJ(object);
	}
}

void Table::trace(GC& gc) {
	for (size_t i = 0; i < m_cap; ++i) {
		Entry& e = m_entries[i];
		if (IS_ENTRY_FREE(e) or IS_ENTRY_DEAD(e)) continue;
		gc.mark_value(e.key);
		gc.mark_value(e.value);
	}
}

void Table::delete_white_string_keys() {
	for (u32 i = 0; i < m_cap; ++i) {
		Entry& entry = m_entries[i];
		if (IS_ENTRY_DEAD(entry) or IS_ENTRY_FREE(entry)) continue;
		if (VYSE_IS_STRING(entry.key) and !VYSE_AS_STRING(entry.key)->marked) {
			TABLE_PLACE_TOMBSTONE(entry);
		}
	}
}

size_t Table::size() const {
	return sizeof(Table) + m_cap * sizeof(Value);
}

bool operator==(const Table::Entry& a, const Table::Entry& b) {
	return a.hash == b.hash and a.key == b.key;
}

} // namespace vyse 