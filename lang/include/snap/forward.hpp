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
class Prototype;
class Closure;

/// Functions that are called from a snap script
/// but are implemented in C++.
using CFunction = int (*)(VM& vm, int argc);

} // namespace snap