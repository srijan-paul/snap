#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "table.hpp"
#include "value.hpp"
#include <functional>

namespace vy {

class UserData final : public Obj {
	using TraceFn = std::function<void(GC& gc, void* t)>;
	using DeleteFn = std::function<void(void* t)>;

  public:
	UserData(void* const data) : Obj{ObjType::user_data}, m_data{data} {}

	[[nodiscard]] const char* to_cstring() const noexcept override {
		return "user-data";
	}

	~UserData() {
		if (delete_fn) delete_fn(m_data);
	}

  protected:
	void trace(GC& gc) override {
		gc.mark_object(m_proto);
		if (trace_fn) trace_fn(gc, m_data);
	}

	size_t size() const override {
		return sizeof(UserData);
	}

  public:
	void* m_data = nullptr;
	Table* m_proto = nullptr;
	TraceFn trace_fn = nullptr;
	DeleteFn delete_fn = nullptr;
};

} // namespace vy
