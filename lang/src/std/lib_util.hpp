#pragma once
#include <forward.hpp>
#include <value.hpp>

namespace vyse::stdlib::util {

void cfn_error(VM& vm, const char* fname, std::string&& message);
void bad_arg_error(VM& vm, const char* fname, int argn, const char* expected_type,
									 const char* received_type);

// add a key with name [name] and value of type cfunction [cfn] to the table [proto]
void add_libfn(VM& vm, Table& proto, const char* name, CFunction cfn);

bool check_arg_type(VM& vm, int argn, ValueType expected_type, const char* fname);
bool check_arg_type(VM& vm, int argn, ObjType expected_type, const char* fname);

}