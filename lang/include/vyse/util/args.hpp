#pragma once
#include <common.hpp>
#include <exception>
#include <forward.hpp>
#include <value.hpp>
#include <vm.hpp>

namespace vyse::util {

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

struct CMiscException : std::exception {
	CMiscException(const char* func, const char* msg) : message(msg), fname(func) {}
	const char* message;
	const char* fname;
};

template <typename...>
constexpr bool AlwaysFalse = false;

/// @brief Maps an `ObjType` enum tag to an actual C++ type that derives from vyse::Object.
template <ObjType Ot>
struct ObjTypeOfTag;

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
/// by this class and the VM.
class Args {
	VYSE_NO_COPY(Args);
	VYSE_NO_DEFAULT_CONSTRUCT(Args);
	VYSE_NO_MOVE(Args);

  public:
	Args(VM& vm, const char* fname, int params, int argc)
		: m_vm(vm), m_fname(fname), m_num_params(params), m_argc(argc) {}

	void check_count(int min_args, int max_args);

	template <typename T>
	T& next() noexcept(false) {
		static_assert(std::is_base_of_v<Obj, T>,
					  "Args.next() can only be used to retreive parameters that are vyse Objects.");
		constexpr ObjType tag = TagOfType<T>::tag;
		return check_and_get<T>(tag);
	}

	Value next_arg() {
		check_arg_overflow();
		return m_vm.get_arg(m_used_argc++);
	}

	number next_number() noexcept(false) {
		return check_and_get<number>(ValueType::Number);
	}

	bool next_bool() noexcept(false) {
		return check_and_get<bool>(ValueType::Bool);
	}

	Obj* next_obj() noexcept(false) {
		return check_and_get<Obj*>(ValueType::Object);
	}

	void check(bool cond, const char* err_msg) noexcept(false) {
		if (not cond) {
			throw CMiscException(err_msg, m_fname);
		}
	}

  private:
	inline void check_arg_overflow() noexcept(false) {
		if (m_used_argc == m_argc) {
			throw CArityException(m_fname, m_num_params);
		}
	}

	template <typename T>
	T& check_and_get(ObjType typ) noexcept(false) {
		check_arg_overflow();
		const Value& arg = m_vm.get_arg(m_used_argc);
		++m_used_argc;

		if (not VYSE_IS_OBJECT(arg)) {
			throw CTypeException(m_fname, m_used_argc, otype_to_string(typ),
								 vtype_to_string(VYSE_GET_TT(arg)));
		}

		if (VYSE_AS_OBJECT(arg)->tag != typ) {
			throw CTypeException(m_fname, m_used_argc, otype_to_string(typ),
								 otype_to_string(VYSE_AS_OBJECT(arg)->tag));
		}

		return *static_cast<T*>(VYSE_AS_OBJECT(arg));
	}

	template <typename T>
	T check_and_get(ValueType tag) noexcept(false) {
		check_arg_overflow();
		Value& arg = m_vm.get_arg(m_used_argc);
		++m_used_argc;

		if (not VYSE_CHECK_TT(arg, tag)) {
			throw CTypeException(m_fname, m_used_argc, vtype_to_string(tag),
								 vtype_to_string(VYSE_GET_TT(arg)));
		}

		return value_get<T>(arg);
	}

	VM& m_vm;
	const char* m_fname;
	const int m_num_params;
	const int m_argc;
	int m_used_argc = 0;
};

} // namespace vyse::util