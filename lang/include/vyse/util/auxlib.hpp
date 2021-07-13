#include "args.hpp"
#include "native_module.hpp"

/// @file A bunch of helper functions to assist library authors who want to
/// write native C++ extensions to vyse.

namespace vyse::util {

/// @brief returns compile time size of a C-style array.
template <typename T, std::size_t N>
constexpr std::size_t array_size(T const (&)[N]) noexcept {
	return N;
}

} // namespace vyse::util