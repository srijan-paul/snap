#pragma once
#include "../../forward.hpp"

namespace vyse::stdlib::primitives {

void load_list_proto(VM& vm);

Value foreach (VM&, int);
Value slice(VM&, int);
Value map(VM&, int);
Value reduce(VM&, int);

} // namespace vyse::stdlib::primitives