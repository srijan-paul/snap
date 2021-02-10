#include <cstdint>
#include <cstring>
#include <memory>

namespace snap {

using u32 = uint32_t;
using s32 = int32_t;
using u64 = uint64_t;
using s64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;

using number = double;

class VM;
class Compiler;

// If this macro is defined, then snap will use
// NaN tagged values that shorten the value representation
// of the interpreter to a simple 8 byte double, using the NaN
// bits to encode the type information.
// #define SNAP_NAN_TAGGING 1

#define SNAP_DEBUG_RUNTIME 1
// #define SNAP_DEBUG_DISASSEMBLY 1

#define SNAP_STRESS_GC 1
#define SNAP_LOG_GC	   1

#ifndef SNAP_NAN_TAGGING
struct Value;
#endif

} // namespace snap
