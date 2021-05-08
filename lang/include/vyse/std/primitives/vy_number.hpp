#pragma once
#include "../../forward.hpp"

namespace vyse::stdlib::primitives {

/// @brief load and initialize the number prototype table.
void load_num_proto(VM& vm);

/// @brief The __to_str protomethod of numbers that coerces
/// them to strings.
Value to_str(VM&, int);

} // namespace vyse::stdlib::primitives
