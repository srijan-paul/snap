#pragma once
#include <string.hpp>
#include <unordered_map>
#include <value.hpp>

namespace snap {

struct ValueHasher {
	std::size_t hash_object(Obj* object) const;
	std::size_t operator()(const Value& value) const;
};

class Table final : public Obj {
	std::unordered_map<Value, Value, ValueHasher> m_entries;
	Table* m_meta_table = nullptr;

  public:
	explicit Table() : Obj(ObjType::table){};
	Value get(const Value& key) const;
	void set(const Value& key, Value value);
	Value& operator[](std::size_t index);
};

} // namespace snap