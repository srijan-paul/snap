#include <vcruntime.h>
#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "table.hpp"
#include "value.hpp"

namespace vy {

template <typename T>
class UserData final : public Obj {
  public:
	UserData(T* const data) : Obj{ObjType::user_data}, m_data{data} {}

	const char* to_cstring() const {
		return "user-data";
	}

  protected:
	void trace(GC& gc) override {
		gc.mark_object(m_proto);
	}

	size_t size() const override {
		return sizeof(UserData<T>) + sizeof(T);
	}

  public:
	T* const m_data = nullptr;
	Table* m_proto = nullptr;
};

} // namespace vy
