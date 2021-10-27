#pragma once
#include <common.hpp>
#include <exception>
#include <forward.hpp>
#include <string>
#include <type_traits>

namespace vy::util {


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
