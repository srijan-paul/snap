#pragma once
#include <forward.hpp>

namespace vyse {
void bad_arg_error(vyse::VM& vm, const char* fname, int argn, const char* expected_type,
									 const char* received_type);
}