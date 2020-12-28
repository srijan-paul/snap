#include <value.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace snap {

using VT = ValueType;
using OT = ObjType;
using TT = TokenType;

Value::Value(char* s, int len) : tag{VT::Object} {
	as.object = new String(s, len);
}

String* String::concatenate(const String* a, const String* b) {
	size_t length = a->length + b->length;
	char* buf = new char[length + 1];
	buf[length] = '\0';
	std::memcpy(buf, a->chars, a->length);
	std::memcpy(buf + a->length, b->chars, b->length);
	return new String(buf, length);
}

String::~String() {
	free(chars);
}

void print_value(Value v) {
	switch (v.tag) {
	case VT::Int: printf("%zu", SNAP_AS_INT(v)); break;
	case VT::Float: printf("%f", SNAP_AS_FLOAT(v)); break;
	case VT::Bool: printf("%s", (SNAP_AS_BOOL(v) ? "true" : "false")); break;
	case VT::Object:
		if (v.is_string())
			printf("%s", SNAP_AS_CSTRING(v));
		else
			printf("<snap object>");
		break;
	default: printf("Internal error: Unknown value type tag!.\n");
	}
}

std::string Value::name_str() const {
	switch (tag) {
	case VT::Int: return std::to_string(as_int());
	case VT::Float: return std::to_string(as_float());
	case VT::Bool: return as_bool() ? std::string("true") : std::string("false");
	case VT::Object:
		if (is_string()) return std::string(SNAP_AS_CSTRING(*this));
		return "<snap object>";
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