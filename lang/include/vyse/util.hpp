#pragma once
#include <cstdint>

/// from: https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
/// @brief return the power of 2 that is greater than, or equal to [v]
inline std::uint32_t pow2ceil(std::uint32_t v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}