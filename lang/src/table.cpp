#include "value.hpp"
#include <cassert>
#include <table.hpp>
#include <upvalue.hpp>

namespace snap {

using VT = ValueType;
using OT = ObjType;

#define TABLE_GET_SLOT(k, h)	   search_entry<Table, Entry>(this, k, h)
#define TABLE_GET_SLOT_CONST(k, h) search_entry<const Table, const Entry>(this, k, h)
#define TABLE_PLACE_TOMBSTONE(e)   (SNAP_SET_TT(e.key, VT::Empty), e.value = SNAP_NIL_VAL)

// check if an entry is unoccupied.
#define IS_ENTRY_FREE(e) (SNAP_IS_NIL(e.key))

#define HASH_OBJ(o) ((size_t)(o)&UINT64_MAX)

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
		if (IS_ENTRY_FREE(entry) or SNAP_IS_EMPTY(entry.key)) continue;
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
	size_t mask = m_cap - 1;
	size_t hash = hash_value(key);
	size_t index = hash & mask;

	while (true) {
		Entry& entry = m_entries[index];
		if ((entry.hash == hash and entry.key == key) or IS_ENTRY_FREE(entry)) return entry.value;
		index = (index + 1) & mask;
	}

	return m_meta_table == nullptr ? SNAP_NIL_VAL : m_meta_table->get(key);
}

bool Table::set(Value key, Value value) {
	assert(!SNAP_IS_NIL(key));

	// If the value is nil, then the key is
	// simply removed with a tombstone in the
	// table.
	if (SNAP_IS_NIL(value)) return remove(key);

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
		const bool is_tombstone = SNAP_IS_EMPTY(entry.key);

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
	if (IS_ENTRY_FREE(entry) or SNAP_IS_EMPTY(entry.key)) return false;

	TABLE_PLACE_TOMBSTONE(entry);
	++m_num_tombstones;
	return true;
}

size_t Table::size() const {
	return m_num_entries - m_num_tombstones;
}

String* Table::find_string(const char* chars, size_t length, size_t hash) {
	assert(chars != nullptr);
	assert(hash == hash_cstring(chars, length));

	size_t mask = m_cap - 1;
	size_t index = hash & mask;

	while (true) {
		Entry& entry = m_entries[index];

		if (entry.hash == hash) {
			Value& k = entry.key;
			if (SNAP_IS_STRING(k)) {
				String* s = SNAP_AS_STRING(k);
				if (s->m_length == length and std::memcmp(s->c_str(), chars, length) == 0) return s;
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
	assert(SNAP_GET_TT(key) != VT::Nil);
	switch (SNAP_GET_TT(key)) {
	case VT::Bool: return SNAP_AS_BOOL(key) ? 7 : 15;
	case VT::Number: return size_t(SNAP_AS_NUM(key)); // TODO: use a proper numeric hash
	case VT::Object: return hash_object(SNAP_AS_OBJECT(key));
	default: return -1; // impossible.
	}
}

size_t Table::hash_object(Obj* object) const {
	switch (object->tag) {
	case OT::string: return static_cast<String*>(object)->m_hash;
	case OT::upvalue: return hash_value(*static_cast<Upvalue*>(object)->value);
	default: return HASH_OBJ(object);
	}
}

bool operator==(const Table::Entry& a, const Table::Entry& b) {
	return a.hash == b.hash and a.key == b.key;
}

} // namespace snap