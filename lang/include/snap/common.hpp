#include <cstdint>
#include <cstring>
#include <memory>

namespace snap {

using u32 = uint32_t;
using s32 = int32_t;
using u64 = uint64_t;
using s64 = int64_t;
using u8  = uint8_t;
using u16 = uint16_t;

using number = double;

#define SNAP_MODE_DEBUG
// #define SNAP_DEBUG_RUNTIME 1
// #define SNAP_DEBUG_DISASSEMBLY 1

#ifdef SNAP_MODE_DEBUG
#define SNAP_ASSERT(cond, message)                                                                 \
	do {                                                                                           \
		if (!(cond)) {                                                                             \
			fprintf(stderr, "[%s:%d]: ASSERTION FAILED!: %s", __func__, __LINE__, message);        \
			abort();                                                                               \
		}                                                                                          \
	} while (false);
#else
#define SNAP_ASSERT(cond, message)                                                                 \
	do {                                                                                           \
	} while (false);
#endif

#define SNAP_STRESS_GC 1
#define SNAP_LOG_GC	   1

} // namespace snap
