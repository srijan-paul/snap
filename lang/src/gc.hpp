#include <vm.hpp>
#include <value.hpp>

namespace snap::GC {

void mark_roots(VM& vm);
void trace_refs(VM& vm);
void sweep(VM& vm);

void free_object(Obj* object);

} // namespace snap::GC
