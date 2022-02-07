#pragma once
#include <common.hpp>
#include <exception>
#include <forward.hpp>
#include <stdexcept>
#include <value.hpp>
#include <vm.hpp>

namespace vy::util {

/// @brief The exception thrown when a NativeFn has incorrect number of arguments.
/// This is handled in the VM.
struct CArityException : std::exception {
	CArityException(const char* fname, int nparams) : cfunc_name(fname), num_params(nparams) {}
	const char* cfunc_name;
	const int num_params;
};

/// @brief A CTypeException is thrown when A NativeFn has an incorect argument type.
struct CTypeException : std::exception {
	CTypeException(const char* fname, int arg_n, const char* expect, const char* received)
		: func_name(fname), argn(arg_n), expected_type_name(expect), received_type_name(received) {}

	const char* func_name;
	const int argn;
	const char* expected_type_name;
	const char* received_type_name;
};

struct CMiscException : std::runtime_error {
	CMiscException(const char* func, const char* msg) : std::runtime_error(msg), fname(func) {}
	const char* fname;
};

template <typename...>
constexpr bool AlwaysFalse = false;

template <typename T>
struct TagOfType {
	static constexpr ObjType tag = ObjType::user_data;
};

#define DECL_TAG_MAPPING(type, objtag)                                                             \
	template <>                                                                                    \
	struct TagOfType<type> {                                                                       \
		static constexpr auto tag = objtag;                                                        \
	}

DECL_TAG_MAPPING(String, ObjType::string);
DECL_TAG_MAPPING(CodeBlock, ObjType::codeblock);
DECL_TAG_MAPPING(List, ObjType::list);
DECL_TAG_MAPPING(Closure, ObjType::closure);
DECL_TAG_MAPPING(CClosure, ObjType::c_closure);
DECL_TAG_MAPPING(Table, ObjType::table);
DECL_TAG_MAPPING(Upvalue, ObjType::upvalue);

DECL_TAG_MAPPING(number, ValueType::Number);
DECL_TAG_MAPPING(bool, ValueType::Bool);
DECL_TAG_MAPPING(Obj*, ValueType::Object);

#undef DECL_TAG_MAPPING

/// @brief A helper class that can be used in Native functions to fetch arguments without having
/// to worry about type checking. All argument count or type checking and error reporting is handled
/// by this class and the VM. look at functions in `lang/src/vymath.cpp` for usage reference.
class Args {
	VYSE_NO_COPY(Args);
	VYSE_NO_DEFAULT_CONSTRUCT(Args);
	VYSE_NO_MOVE(Args);

  public:
	Args(VM& vm, const char* fname, int params, int argc)
		: m_vm(vm), m_fname(fname), m_num_params(params), m_argc(argc) {}

	void check_count(int min_args, int max_args);

	template <typename T>
	[[nodiscard]] T& next() noexcept(false) {
		static_assert(std::is_base_of_v<Obj, T>,
					  "Args.next() can only be used to retreive parameters that are vyse Objects.");
		constexpr ObjType tag = TagOfType<T>::tag;
		return check_and_get<T>(tag);
	}

	template <typename T>
	T* next_udata_arg() noexcept(false) {
		const Value arg = next_arg();
		if (!VYSE_IS_OBJECT(arg)) {
			throw CTypeException(m_fname, m_used_argc, typeid(T).name(),
								 vtype_to_string(VYSE_GET_TT(arg)));
		}

		Obj* const object = VYSE_AS_OBJECT(arg);
		if (object->tag != ObjType::user_data || !static_cast<UserData*>(object)->is_of_type<T>()) {
			throw CTypeException(m_fname, m_used_argc, typeid(T).name(), object->to_cstring());
		}

		UserData* const udata = static_cast<UserData*>(object);
		// we can use `unsafe_get` here since we've already checked `is_of_type` above.
		return udata->unsafe_get<T>();
	}

	[[nodiscard]] Value next_arg() noexcept(false) {
		check_arg_overflow();
		return m_vm.get_arg(m_used_argc++);
	}

	[[nodiscard]] number next_number() noexcept(false) {
		return check_and_get<number>(ValueType::Number);
	}

	[[nodiscard]] bool next_bool() noexcept(false) {
		return check_and_get<bool>(ValueType::Bool);
	}

	[[nodiscard]] Obj* next_obj() noexcept(false) {
		return check_and_get<Obj*>(ValueType::Object);
	}

	void check(bool cond, std::string_view err_msg) noexcept(false) {
		if (not cond) {
			throw CMiscException(m_fname, err_msg.data());
		}
	}

	[[nodiscard]] inline bool has_next() const noexcept {
		return m_argc != m_used_argc;
	}

  private:
	inline void check_arg_overflow() noexcept(false) {
		if (m_used_argc == m_argc) {
			throw CArityException(m_fname, m_num_params);
		}
	}

	template <typename T>
	[[nodiscard]] T& check_and_get(ObjType type_tag) noexcept(false) {
		check_arg_overflow();
		const Value& arg = m_vm.get_arg(m_used_argc);
		++m_used_argc;

		if (not VYSE_IS_OBJECT(arg)) {
			throw CTypeException(m_fname, m_used_argc, otype_to_string(type_tag),
								 vtype_to_string(VYSE_GET_TT(arg)));
		}

		Obj* const object = VYSE_AS_OBJECT(arg);
		if (object->tag != type_tag) {
			throw CTypeException(m_fname, m_used_argc, otype_to_string(type_tag),
								 value_type_name(arg));
		}

		return *static_cast<T*>(VYSE_AS_OBJECT(arg));
	}

	template <typename T>
	[[nodiscard]] T check_and_get(ValueType tag) noexcept(false) {
		check_arg_overflow();
		Value& arg = m_vm.get_arg(m_used_argc);
		++m_used_argc;

		if (not VYSE_CHECK_TT(arg, tag)) {
			throw CTypeException(m_fname, m_used_argc, vtype_to_string(tag), value_type_name(arg));
		}

		return value_get<T>(arg);
	}

	VM& m_vm;
	const char* m_fname;
	const int m_num_params;
	const int m_argc;
	int m_used_argc = 0;
};

} // namespace vy::util
