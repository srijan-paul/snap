#include <cassert>
#include <cstdio>
#include <cstring>
#include <value.hpp>
#include <vm.hpp>

// this hash function is from: https://craftinginterpreters.com/hash-tables.html
static uint32_t fnv1a(const char* key, int length) {
	uint32_t hash = 2166136261u;

	for (int i = 0; i < length; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}

namespace snap {

using VT = ValueType;
using OT = ObjType;
using TT = TokenType;

Obj::Obj(VM& vm, OT type): tag{type} {
	vm.register_object(this);
}

s32 Obj::hash() {
	m_hash = (std::size_t)(this);
	return m_hash;
}

s32 String::hash() {
	m_hash = fnv1a(chars, length);
	return m_hash;
}

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
	case VT::Number: std::printf("%f", SNAP_AS_NUM(v)); break;
	case VT::Bool: std::printf("%s", (SNAP_AS_BOOL(v) ? "true" : "false")); break;
	case VT::Object:
		if (SNAP_IS_STRING(v))
			std::printf("%s", SNAP_AS_CSTRING(v));
		else
			std::printf("<snap object>");
		break;
	default:
		std::printf("Internal error: Impossible value type tag %zu!.\n",
					static_cast<size_t>(v.tag));
	}
}

std::string Value::name_str() const {
	switch (tag) {
	case VT::Number: return std::to_string(as_num());
	case VT::Bool: return as_bool() ? std::string("true") : std::string("false");
	case VT::Nil: return "nil";
	case VT::Object:
		if (is_string()) return std::string(SNAP_AS_CSTRING(*this));
		return "<snap object>";
	default: return "<unknown value>";
	}
}

const char* Value::type_name() const {
	switch (tag) {
	case VT::Number: return "number";
	case VT::Bool: return "bool";
	case VT::Object: {
		if (is_string()) return "string";
		return "object";
	}
	case VT::Nil: return "nil";
	default: return "unknown";
	}
}

bool Value::are_equal(Value a, Value b) {
	if (a.tag != b.tag) return false;
	switch (a.tag) {
	case VT::Number: return b.as_num() == a.as_num();
	case VT::Bool: return a.as_bool() == b.as_bool();
	case VT::Object: return a.as_object() == b.as_object();
	default: return false;
	}
}

} // namespace snap
