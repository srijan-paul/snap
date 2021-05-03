#pragma once
#include <cstdint>
#include <cstring>
#include <memory>

namespace vyse {

constexpr int VersionMajor = 0;
constexpr int VersionMinor = 0;
constexpr int VersionPatch = 1;

constexpr const char* VersionCString = "0.0.1";

using u32 = std::uint32_t;
using s32 = std::int32_t;
using u64 = std::uint64_t;
using s64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;

using std::size_t;
using number = double;

#define VYSE_DEBUG 
// #define VYSE_DEBUG_RUNTIME		 1
#define VYSE_DEBUG_DISASSEMBLY 1

#ifdef VYSE_DEBUG

#define VYSE_ASSERT(cond, message)                                                                 \
	((cond) ? 0                                                                                      \
					: (fprintf(stderr, "[%s:%d]: ASSERTION FAILED!: %s", __func__, __LINE__, message),       \
						 abort(), 0))

#define VYSE_ERROR(message)                                                                        \
	(fprintf(stderr, "[%s:%d]: Interal Error: %s", __func__, __LINE__, message), abort())

#define VYSE_UNREACHABLE() VYSE_ERROR("Unreachable code point reached.")

#else

#define VYSE_ASSERT(cond, message) 0
#define VYSE_ERROR(message)				 0

#endif

#define VYSE_NO_COPY(class)							 class(class const& other) = delete
#define VYSE_NO_MOVE(class)							 class(class && other) = delete
#define VYSE_NO_DEFAULT_CONSTRUCT(class) class() = delete

#define VYSE_STRESS_GC 1
// #define VYSE_LOG_GC		 1

} // namespace vyse
