#pragma once
#include "value.hpp"

namespace vyse {

struct Upvalue final : public Obj {
public:
	explicit Upvalue(Value* v) noexcept : Obj(ObjType::upvalue), m_value{v} {};
	Value* m_value;								 // points to a stack slot until closed.
	Value closed = VYSE_NIL_VAL;	 // The value is stored here upon closing.
	Upvalue* next_upval = nullptr; // next upvalue in the VM's upvalue list.

private:
	void trace(GC& gc) override;
	size_t size() const override {
		return sizeof(Upvalue);
	}
};

} // namespace vyse