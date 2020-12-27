#include "value.hpp"
#include <cstdio>

namespace snap {

using VT = ValueType;
void print_value(Value v) {
	switch (v.tag) {
	case VT::Int: printf("%zu", SNAP_AS_INT(v)); break;
	case VT::Float: printf("%f", SNAP_AS_FLOAT(v)); break;
	case VT::Bool: printf("%s", (SNAP_AS_BOOL(v) ? "true" : "false"));
	default: printf("Internal error: Unknown value type tag!.\n");
	}
}

std::string Value::name_str() const {
	switch (tag) {
	case VT::Int: return std::to_string(as_int());
	case VT::Float: return std::to_string(as_float());
	case VT::Bool: return as_bool() ? std::string("true") : std::string("false");
	default: return "Unknown type tag!";
	}
}

bool Value::are_equal(Value a, Value b) {
	switch (a.tag) {
	case VT::Float:
		if (b.tag == VT::Float) return SNAP_AS_FLOAT(a) == SNAP_AS_FLOAT(b);
		if (b.tag == VT::Int) return SNAP_AS_FLOAT(a) == SNAP_AS_INT(b);
		return false;
	case VT::Int:
		if (b.tag == VT::Float) return SNAP_AS_INT(a) == SNAP_AS_FLOAT(b);
		if (b.tag == VT::Int) return SNAP_AS_INT(a) == SNAP_AS_INT(b);
		return false;
	case VT::Bool: return SNAP_AS_BOOL(a) == SNAP_AS_BOOL(b);
	default: return false;
	}
}

} // namespace snap