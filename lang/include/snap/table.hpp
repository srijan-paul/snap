#pragma once
#include <string.hpp>
#include <unordered_map>
#include <value.hpp>

namespace snap {

struct ValueHasher {
  std::size_t hash_object(Obj* object) const;
	std::size_t operator()(const Value& value) const;
};

class Table : public Obj {
	std::unordered_map<Value, Value, ValueHasher> m_entries;

  public:
	Value get(const Value& key) const;
	Value set(const Value& key, Value value);
};

} // namespace snap