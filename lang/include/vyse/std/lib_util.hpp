#pragma once
#include "table.hpp"
#include <common.hpp>
#include <exception>
#include <forward.hpp>
#include <function.hpp>
#include <type_traits>
#include <value.hpp>
#include <vm.hpp>
#include <vy_list.hpp>

namespace vyse::stdlib::util {

/// @brief The exception thrown when a NativeFunction has incorrect number of arguments.
/// This is handled in the VM.
struct CArityException : std::exception {
	CArityException(const char* fname, int nparams) : cfunc_name(fname), num_params(nparams) {}
	const char* cfunc_name;
	int num_params;
};

struct CTypeException : std::exception {
	CTypeException(const char* fname, const char* expect, const char* received)
		: func_name(fname), expected_type_name(expect), received_type_name(received) {}

	const char* func_name;
	const char* expected_type_name;
	const char* received_type_name;
};

template <typename...>
constexpr bool AlwaysFalse = false;

template <ObjType Ot>
struct ObjTypeFromTag;

#define DECL_OBJTYPE_MAPPING(tag, typ)                                                             \
	template <>                                                                                    \
	struct ObjTypeFromTag<tag> {                                                                   \
		using type = typ;                                                                          \
	}

DECL_OBJTYPE_MAPPING(ObjType::string, String);
DECL_OBJTYPE_MAPPING(ObjType::codeblock, CodeBlock);
DECL_OBJTYPE_MAPPING(ObjType::list, List);
DECL_OBJTYPE_MAPPING(ObjType::closure, Closure);
DECL_OBJTYPE_MAPPING(ObjType::c_closure, CClosure);
DECL_OBJTYPE_MAPPING(ObjType::table, Table);
DECL_OBJTYPE_MAPPING(ObjType::upvalue, Upvalue);

template <ObjType T>
using ObjTypeFromTag_t = typename ObjTypeFromTag<T>::type;

#undef DECL_OBJTYPE

class CFuncHelper {
	VYSE_NO_COPY(CFuncHelper);
	VYSE_NO_DEFAULT_CONSTRUCT(CFuncHelper);
	VYSE_NO_MOVE(CFuncHelper);

  public:
	CFuncHelper(VM& vm, const char* fname, int params, int argc)
		: m_vm(vm), m_fname(fname), m_num_params(params), m_argc(argc) {}

	/// TODO: do this thing
	/// Value get(ValueType type);

	template <ValueType TagType, typename T>
	T get() {
		return VYSE_NIL;
	}

	template <ObjType TagType>
	ObjTypeFromTag_t<TagType>& get() noexcept(false) {
		/// TODO: fix this once I have userdata
		static_assert(TagType != ObjType::user_data, "UserData not implemented.");
		return check_and_get<ObjTypeFromTag_t<TagType>>(TagType);
	}

  private:
	template <typename T>
	T& check_and_get(ObjType typ) noexcept(false) {
		if (m_used_argc == m_argc) {
			throw CArityException(m_fname, m_num_params);
		}

		const Value& arg = m_vm.get_arg(m_used_argc);
		++m_used_argc;

		if (not VYSE_IS_OBJECT(arg)) {
			throw CTypeException{m_fname, otype_to_string(typ), vtype_to_string(VYSE_GET_TT(arg))};
		}

		if (VYSE_AS_OBJECT(arg)->tag != typ) {
			throw CTypeException{m_fname, otype_to_string(typ), vtype_to_string(VYSE_GET_TT(arg))};
		}

		return *static_cast<T*>(VYSE_AS_OBJECT(arg));
	}

	VM& m_vm;
	const char* m_fname;
	int m_num_params;
	int m_argc;
	int m_used_argc = 0;
};

/// @brief throws an error having the message [message] inside the function having name[fname].
void cfn_error(VM& vm, const char* fname, std::string&& message);
void bad_arg_error(VM& vm, const char* fname, int argn, const char* expected_type,
				   const char* received_type);

/// @brief add a key with name [name] and value of type cfunction [cfn] to the table [proto]
void add_libfn(VM& vm, Table& proto, const char* name, NativeFn cfn);

/// @brief Reports an error and returns false if the native function with name [fname] does not
/// have between [min_args] and [max_args] arguments. When [max_args] is not provided, then it
/// checks if [fname] recieved exactly [min_arg] arguments.
/// @param vm The vm to use when reporting an error.
/// @param fname Name of the native function.
/// @param argc The number of arguments recieved.
/// @param min_args The minimum number of arguments expected.
/// @param max_args The maximum number of arguments expected.
bool check_argc(VM& vm, const char* fname, int argc, int min_args, int max_args = -1);

/// @brief checks the [argn]th argument and throws an error if it's type isn't [expected_type]
/// Note that the arguments are indexed from 0. So argn for the first argument is 0.
/// @param argn Index of the argument. Starting from 0.
/// @param expected_type Expected type to check against.
/// @param fname name of the function in which the argument is being checked.
bool check_arg_type(VM& vm, int argn, ValueType expected_type, const char* fname);

/// @brief checks the [argn]th argument and throws an error if it's type isn't [expected_type]
/// Note that the arguments are indexed from 0. So argn for the first argument is 0.
/// @param argn Index of the argument. Starting from 0.
/// @param expected_type Expected type to check against.
/// @param fname name of the function in which the argument is being checked
bool check_arg_type(VM& vm, int argn, ObjType expected_type, const char* fname);

} // namespace vyse::stdlib::util
