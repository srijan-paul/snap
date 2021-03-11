#pragma once
#include "value.hpp"

namespace snap {

struct Upvalue final : Obj {
	Value* m_value;				   // points to a stack slot until closed.
	Value closed = SNAP_NIL_VAL;   // The value is stored here upon closing.
	Upvalue* next_upval = nullptr; // next upvalue in the VM's upvalue list.
	Upvalue(Value* v) : Obj(ObjType::upvalue), m_value{v} {};
};

} // namespace snap