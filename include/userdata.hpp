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

	size_t const m_type_id;
	Table* m_proto = nullptr;
	TraceFn m_tracer = nullptr;
	DeleteFn m_deleter = nullptr;
	CClosure* m_indexer = nullptr;
};

template <typename T>
class UserData final : public UserDataBase {
  public:
	static const size_t TypeId;
	
	UserData() = delete;
	UserData(T* const data) : UserDataBase(typeid(T).hash_code()), m_data{data} {}

	~UserData() {
		if (!m_deleter) return;
		if (typeid(T).hash_code() == m_type_id) {
			m_deleter(m_data);
		}
	}

	[[nodiscard]] const char* to_cstring() const noexcept override {
		return "user-data";
	}

	[[nodiscard]] static size_t get_type_id() noexcept {
		return typeid(T).hash_code();
	}

  protected:
	void trace(GC& gc) override {
		gc.mark_object(m_proto);
		gc.mark_object(m_indexer);
		if (m_tracer) {
			if (get_type_id() == m_type_id) {
				m_tracer(gc, m_data);
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
