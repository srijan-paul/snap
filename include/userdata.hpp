#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "table.hpp"
#include "value.hpp"
#include <functional>
#include <typeinfo>

namespace vy {

class UserData : public Obj {
	VYSE_NO_DEFAULT_CONSTRUCT(UserData);

	using TraceFn = void(GC& gc, void* t);
	using DeleteFn = void(void* data);

  public:
	// clang-format off
	UserData(UserData&& other)
		: Obj{ObjType::user_data},
			m_type_id{other.m_type_id},
			m_proto{other.m_proto},
			m_data{other.m_data} {
		other.m_data = nullptr;
		other.m_proto = nullptr;
	}
	// clang-format on

	template <typename T>
	[[nodiscard]] inline static UserData* make(T* data, Table* proto = nullptr) {
		return new UserData(typeid(T).hash_code(), data, proto);
	}

	~UserData() {
		if (m_deleter) {
			m_deleter(m_data);
		}
	}

	/// @brief Returns `true` if this UserData contains data of type `T`.
	template <typename T>
	[[nodiscard]] constexpr bool is_of_type() const noexcept {
		return typeid(T).hash_code() == m_type_id;
	}

	/// @brief If `T` is the type of the data contained in this wrapper, then returns
	/// a pointer to the data. Else returns nullptr.
	template <typename T>
	[[nodiscard]] constexpr T* get() const noexcept {
		if (is_of_type<T>()) return static_cast<T*>(m_data);
		return nullptr;
	}

	/// @brief casts the data stored in this wrapper to `T` and returns it.
	/// This cast is unsafe since no runtime check is performed to see if `T` and
	/// the type of the data are consistent.
	template <typename T>
	[[nodiscard]] constexpr T* unsafe_get() const noexcept {
		return static_cast<T*>(m_data);
	}

	template <typename T>
	[[nodiscard]] constexpr bool set(T* const new_data) const noexcept {
		if (is_of_type<T>()) {
			m_data = new_data;
			return true;
		}
		return false;
	}

	[[nodiscard]] size_t size() const override {
		return sizeof(UserData);
	}

	[[nodiscard]] const char* to_cstring() const override {
		return "userdata";
	}

  protected:
	void trace(GC& gc) override {
		gc.mark(m_proto);
		if (m_tracer) {
			m_tracer(gc, m_data);
		}
	}

  private:
	// clang-format off
	UserData(size_t type_id, void* const data, Table* const proto = nullptr)
		: Obj{ObjType::user_data},
			m_type_id{type_id},
			m_proto{proto},
			m_data{data} {}
	// clang-format on

  public:
	/// @brief An ID that uniquely identifies the datatype that this UserData wraps.
	size_t const m_type_id;

	/// @brief Prototype for this UserData.
	Table* m_proto = nullptr;

	/// @brief A function used to trace this object when the GC cycle is triggered.
	TraceFn* m_tracer = nullptr;

	/// @brief A function used to destruct the data stored before deleting the wrapping Userdata.
	DeleteFn* m_deleter = nullptr;

	/// @brief A table which is consulted when the user wants to index this userdata.
	/// This serves as a "proxy". Any property accesses done on the userdata are routed to this.
	/// If no value is found, then it is routed to the prototype.
	Table* indexer = nullptr;

  private:
	void* m_data = nullptr;
};

} // namespace vy
