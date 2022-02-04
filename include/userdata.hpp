#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "table.hpp"
#include "value.hpp"
#include <functional>
#include <typeinfo>

namespace vy {

struct UserDataBase : public Obj {
	using TraceFn = std::function<void(GC& gc, void* t)>;
	using DeleteFn = std::function<void(void* t)>;

	UserDataBase(size_t type_id, Table* proto)
		: Obj{ObjType::user_data}, m_type_id{type_id}, m_proto{proto} {}
	UserDataBase(size_t type_id) : Obj{ObjType::user_data}, m_type_id{type_id} {}

	std::size_t const m_type_id;
	Table* m_proto = nullptr;
	TraceFn trace_fn = nullptr;
	DeleteFn delete_fn = nullptr;
};

template <typename T>
class UserData final : public UserDataBase {
  public:
	UserData() = delete;
	UserData(T* const data) : UserDataBase(typeid(T).hash_code()), m_data{data} {}

	~UserData() {
		if (!delete_fn) return;
		if (typeid(T).hash_code() == m_type_id) {
			delete_fn(m_data);
		} else {
			throw "Bad";
		}
	}

	[[nodiscard]] const char* to_cstring() const noexcept override {
		return "user-data";
	}

  protected:
	void trace(GC& gc) override {
		gc.mark_object(m_proto);
		if (trace_fn) {
			if (typeid(T).hash_code() == m_type_id) {
				trace_fn(gc, m_data);
			}
		}
	}

	size_t size() const override {
		return sizeof(UserData<T>);
	};

  public:
	T* m_data;
};

} // namespace vy
