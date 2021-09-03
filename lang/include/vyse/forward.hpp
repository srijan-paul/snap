/// Forward declarations to avoid name resolution conflicts.

namespace vyse {

#ifndef VYSE_NAN_TAG
struct Value;
#else
using Value = double;
#endif

class Compiler;
class VM;
class GC;

class Obj;
class String;
class Table;
class List;
class CodeBlock;
class Closure;
class CClosure;
class Upvalue;

enum class ObjType : unsigned char;
enum class ValueType : unsigned char;

/// @brief Functions that are called from a snap script but are implemented in C++.
using NativeFn = Value (*)(VM& vm, int argc);

/// @brief Functions that load a library authored in C++
using LibInitFn = Table& (*)(VM& vm);

} // namespace vyse