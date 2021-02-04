#include <cassert>
#include <table.hpp>
#include <upvalue.hpp>

namespace snap {

using VT = ValueType;
using OT = ObjType;

std::size_t ValueHasher::operator()(const Value& key) const {
	assert(key.tag != VT::Nil);
	switch (SNAP_GET_TT(key)) {
	case VT::Bool: return SNAP_AS_BOOL(key) ? 1 : 0;
	case VT::Number: return std::size_t(SNAP_AS_NUM(key));
	case VT::Object: return hash_object(SNAP_AS_OBJECT(key));
	default: return -1; // impossible.
	}
}

std::size_t ValueHasher::hash_object(Obj* object) const {
	switch (object->tag) {
	case OT::string: return static_cast<String*>(object)->m_hash;
	case OT::upvalue: return this->operator()(*static_cast<Upvalue*>(object)->value);
	default:
		if (object->m_hash == -1) return object->hash();
		return object->m_hash;
	}
}

Value Table::get(const Value& key) const {
	auto search = m_entries.find(key);
	return (search == m_entries.end()) ? SNAP_NIL_VAL : search->second;
}

void Table::set(const Value& key, Value value) {
	m_entries[key] = value;
}

} // namespace snap