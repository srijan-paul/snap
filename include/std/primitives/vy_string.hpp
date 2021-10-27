#pragma once
#include "../../forward.hpp"

namespace vy::stdlib::primitives {

/// Loads the string prototype table and instantiates all it's functions.
void load_string_proto(VM& vm);

/// args: string, from, [len]
/// if to is provided, returns the [len] consecutive characters in [string]
/// starting from [from]. Other wise truncates the characters in range [0, from) and
/// returns the rest.
Value substr(VM&, int);

/// args: string, index
/// returns the ascii code of the character at position [index] in [string].
Value code_at(VM&, int);

} // namespace vyse::stdlib::primitives
