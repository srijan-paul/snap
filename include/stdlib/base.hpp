#pragma once
#include <forward.hpp> 

namespace vy::stdlib {

Value print(VM&, int);

/// @brief read a line of input from stdin and return as a string.
Value input(VM&, int);

Value setproto(VM&, int);
Value getproto(VM&, int);

Value assert_(VM&, int);
Value import(VM&, int);

} // namespace vy::stdlib
