#include <cassert>
#include <cstdio>
#include <cstring>
#include <function.hpp>
#include <string.hpp>
#include <value.hpp>
#include <vm.hpp>

namespace snap {

using VT = ValueType;
using OT = ObjType;

s32 Obj::hash() {
	m_hash = (std::size_t)(this);
	return m_hash;
}

void print_value(Value v) {
	std::printf("%s", v.name_str().c_str());
}

std::string Value::name_str() const {
	switch (tag) {
	case VT::Number: return std::to_string(as_num());
	case VT::Bool: return as_bool() ? "true" : "false";
	case VT::Nil: return "nil";
	case VT::Object: {
		const Obj* obj = as_object();

		switch (obj->tag) {
		case OT::string: return SNAP_AS_CSTRING(*this);
		case OT::func:
			return std::string("[function ") + static_cast<const Function*>(obj)->name_cstr() + "]";
		case OT::proto:
			return std::string("[prototype ") + static_cast<const Prototype*>(obj)->name_cstr() +
				   "]";
		case OT::upvalue: return static_cast<const Upvalue*>(obj)->value->name_str();
		case OT::table: return "[table]";

		default: return "<snap object>";
		}
	}
	default: return "<unknown value>";
	}
}

const char* Value::type_name() const {
	switch (tag) {
	case VT::Number: return "number";
	case VT::Bool: return "bool";
	case VT::Object: {
		const Obj* obj = as_object();
		if (is_string()) return "string";
		if (obj->tag == OT::func) return "function";
		return "object";
	}
	case VT::Nil: return "nil";
	default: return "unknown";
	}
}

bool operator==(const Value& a, const Value& b) {
	if (a.tag != b.tag) return false;
	switch (a.tag) {
	case VT::Number: return b.as_num() == a.as_num();
	case VT::Bool: return a.as_bool() == b.as_bool();
	case VT::Object: return a.as_object() == b.as_object();
	default: return false;
	}
}

bool operator!=(const Value& a, const Value& b) {
	return !(a == b);
}

} // namespace snap
