#include "args.hpp"
#include "native_module.hpp"

namespace vyse::util {

template <typename T, std::size_t N>
constexpr std::size_t array_size(T const (&)[N]) noexcept {
	return N;
}

} // namespace vyse::util