#pragma once
#include "../../forward.hpp"

namespace vyse::stdlib::primitives {

void load_list_proto(VM& vm);

/// @brief creates a list with an initial size.
Value make(VM&, int);

Value fill(VM&, int);
Value foreach (VM&, int);
Value slice(VM&, int);
Value map(VM&, int);
Value reduce(VM&, int);
Value filter(VM&, int);

} // namespace vyse::stdlib::primitives