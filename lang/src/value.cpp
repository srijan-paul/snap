#include "common.hpp"
#include <cassert>
#include <cstdio>
#include <vm.hpp>

#define CONST_CAST(typ, val) (static_cast<const typ*>(val))

namespace snap {

using VT = ValueType;
using OT = ObjType;

const char* Obj::to_cstring() const {
	return "[snap object]";
}

void print_value(Value v) {
	std::printf("%s", value_to_string(v).c_str());
}

/// Convert a snap number to a cstring.
static char* num_to_cstr(Value v) {
	SNAP_ASSERT(SNAP_IS_NUM(v), "not a number.");
	number num = SNAP_AS_NUM(v);
	// If a whole number, then omit decimal part
	bool is_whole = num == size_t(num);
	const char* fmt = is_whole ? "%d" : "%f";
	int bufsize = std::snprintf(nullptr, 0, fmt, num) + 1;

	char* buf = new char[bufsize]; // + 1 for null terminator.
	int res = std::sprintf(buf, fmt, num);
	SNAP_ASSERT(res > 0, "sprintf failed!");

	return buf;
}

char* value_to_cstring(Value v) {
	switch (SNAP_GET_TT(v)) {
	case VT::Number: {
		return num_to_cstr(v);
	}

	default: SNAP_UNREACHABLE();
	}
}

std::string value_to_string(Value v) {
	switch (SNAP_GET_TT(v)) {
	case VT::Number: return std::to_string(SNAP_AS_NUM(v));
	case VT::Bool: return SNAP_AS_BOOL(v) ? "true" : "false";
	case VT::Nil: return "nil";
	case VT::Empty: return "undefined";
	case VT::Object: {
		const Obj* obj = SNAP_AS_OBJECT(v);

		switch (obj->tag) {
		case OT::string: return SNAP_AS_CSTRING(v);
		case OT::func: return std::string("[fn ") + static_cast<const Closure*>(obj)->name_cstr() + "]";
		case OT::proto:
			return std::string("[prototype ") + static_cast<const Prototype*>(obj)->name_cstr() + "]";
		case OT::upvalue: {
			const Upvalue* upval = static_cast<const Upvalue*>(obj);
			return value_to_string(*(upval->m_value));
		}
		case OT::table: {
			Table* tbl = SNAP_AS_TABLE(v);
			return "[table " + std::to_string((size_t)tbl) + "]";
		}

		default: return "[ snap object ]";
		}
	}
	default: SNAP_ERROR("Impossible value tag.");
	}
}

const char* value_type_name(Value v) {
	switch (SNAP_GET_TT(v)) {
	case VT::Number: return "number";
	case VT::Bool: return "boolean";
	case VT::Object: {
		const Obj* obj = SNAP_AS_OBJECT(v);
		if (SNAP_IS_STRING(v)) return "string";
		if (obj->tag == OT::func or obj->tag == OT::cfunc) return "function";
		SNAP_ASSERT(obj->tag == OT::table, "Impossible type tag.");
		return "table";
	}
	case VT::Nil: return "nil";
	case VT::Empty: return "empty";
	}
}

bool operator==(const Value& a, const Value& b) {
	if (a.tag != b.tag) return false;
	switch (a.tag) {
	case VT::Number: return b.as_num() == a.as_num();
	case VT::Bool: return a.as_bool() == b.as_bool();
	case VT::Object: {
		const Obj* oa = a.as_object();
		const Obj* ob = b.as_object();
		if (oa->tag != ob->tag) return false;
		return oa == ob;
	}
	default: return true;
	}
}

bool operator!=(const Value& a, const Value& b) {
	return !(a == b);
}

} // namespace snap
