#pragma once
#include "value.hpp"

namespace vyse {

class Upvalue final : public Obj {
  public:
	explicit constexpr Upvalue(Value* v) noexcept : Obj(ObjType::upvalue), m_value{v} {};
	~Upvalue() = default;
	Value* m_value;				   // points to a stack slot until closed.
	Value closed = VYSE_NIL;	   // The value is stored here upon closing.
	Upvalue* next_upval = nullptr; // next upvalue in the VM's upvalue list.

  private:
	void trace(GC& gc) override;
	size_t size() const override {
		return sizeof(Upvalue);
	}
};

} // namespace vyse
