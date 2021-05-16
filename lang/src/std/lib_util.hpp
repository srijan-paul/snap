#pragma once
#include <forward.hpp>
#include <value.hpp>

namespace vyse::stdlib::util {

/// @brief throws an error having the message [message] inside the function having name[fname].
void cfn_error(VM& vm, const char* fname, std::string&& message);
void bad_arg_error(VM& vm, const char* fname, int argn, const char* expected_type,
									 const char* received_type);

/// @brief add a key with name [name] and value of type cfunction [cfn] to the table [proto]
void add_libfn(VM& vm, Table& proto, const char* name, NativeFn cfn);

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

}