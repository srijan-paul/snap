/// Forward declarations to avoid name resolution
/// conflicts.

namespace snap {

#ifndef SNAP_NAN_TAG
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
class CodeBlock;
class Closure;

/// Functions that are called from a snap script
/// but are implemented in C++.
using CFunction = Value (*)(VM& vm, int argc);

} // namespace snap