#include <cassert>
#include <cstdio>
#include <cstring>
#include <value.hpp>

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
	case VT::Int: std::printf("%zu", SNAP_AS_INT(v)); break;
	case VT::Float: std::printf("%f", SNAP_AS_FLOAT(v)); break;
	case VT::Bool: std::printf("%s", (SNAP_AS_BOOL(v) ? "true" : "false")); break;
	case VT::Object:
		if (SNAP_IS_STRING(v))
			std::printf("%s", SNAP_AS_CSTRING(v));
		else
			std::printf("<snap object>");
		break;
	default: std::printf("Internal error: Impossible value type tag!.\n");
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

const char* Value::type_name() const {
	switch (tag) {
	case VT::Int: return "int";
	case VT::Float: return "float";
	case VT::Bool: return "bool";
	case VT::Object: {
		if (is_string()) return "string";
		return "object";
	}
	default: return "unknown";
	}
}

bool Value::are_equal(Value a, Value b) {
	switch (a.tag) {
	case VT::Float:
		if (b.tag == VT::Float) return a.as_float() == b.as_float();
		if (b.tag == VT::Int) return a.as_float() == b.as_int();
		return false;
	case VT::Int:
		if (b.tag == VT::Float) return a.as_int() == b.as_float();
		if (b.tag == VT::Int) return a.as_int() == a.as_int();
		return false;
	case VT::Bool: return a.as_bool() == b.as_bool();
	default: return false;
	}
}

} // namespace snap