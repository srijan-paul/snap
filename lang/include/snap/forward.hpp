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
class Function;
class Table;
class Prototype;

} // namespace snap