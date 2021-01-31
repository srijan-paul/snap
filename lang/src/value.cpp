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

Obj::Obj(VM& vm, OT type) : tag{type} {
	vm.register_object(this);
}

s32 Obj::hash() {
	m_hash = (std::size_t)(this);
	return m_hash;
}

String::String(const char* chrs, std::size_t len) : Obj(ObjType::string), length{len} {
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	chars = buf;
}

String::String(VM& vm, const char* chrs, size_t len) : Obj(vm, ObjType::string), length{len} {
	char* buf = new char[len + 1];
	std::memcpy(buf, chrs, len);
	buf[len] = '\0';
	chars = buf;
}

s32 String::hash() {
	m_hash = fnv1a(chars, length);
	return m_hash;
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
	delete[] chars;
}

Function::Function(VM& vm, Prototype* proto_) : Obj(ObjType::func), proto{proto_} {
	vm.register_object(proto_);
	vm.register_object(this);
}

void Function::set_num_upvals(u32 count) {
	num_upvals = count;
	upvals.reserve(count);
}

Function::~Function() {
	for (int i = 0; i < num_upvals; ++i) {
		delete upvals[i];
	}
}

Value::Value(char* s, int len) : tag{VT::Object} {
	as.object = new String(s, len);
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
		case ObjType::string: return SNAP_AS_CSTRING(*this);
		case ObjType::func: {
			return std::string("[function ") +
				   static_cast<const Function*>(obj)->proto->name->chars + "]";
		}
		case ObjType::proto: {
			return std::string("[prototype ") + static_cast<const Prototype*>(obj)->name->chars +
				   "]";
		}
		case ObjType::upvalue: {
			return static_cast<const Upvalue*>(obj)->value->name_str();
		}
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
