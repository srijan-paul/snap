#pragma once
#include "../forward.hpp"

namespace vyse::stdlib {

Value print(VM&, int);
Value setproto(VM&, int);
Value getproto(VM&, int);

Value assert_(VM&, int);

} // namespace vyse::stdlib