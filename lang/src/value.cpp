#include <cassert>
#include <cstdio>
#include <cstring>
#include <function.hpp>
#include <string.hpp>
#include <value.hpp>
#include <vm.hpp>

#define CONST_CAST(typ, val) (static_cast<const typ*>(val))

namespace snap {

using VT = ValueType;
using OT = ObjType;

void print_value(Value v) {
	std::printf("%s", value_to_string(v).c_str());
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
		case OT::func:
			return std::string("[fn ") + static_cast<const Function*>(obj)->name_cstr() + "]";
		case OT::proto:
			return std::string("[prototype ") + static_cast<const Prototype*>(obj)->name_cstr() +
				   "]";
		case OT::upvalue: {
			const Upvalue* upval = static_cast<const Upvalue*>(obj);
			return value_to_string(*(upval->m_value));
		}
		case OT::table: {
			Table* tbl = SNAP_AS_TABLE(v);
			return "[table " + std::to_string((size_t)tbl) + "]";
		}

		default: return "<snap object>";
		}
	}
	default: return "<unknown value>";
	}
}

const char* value_type_name(Value v) {
	switch (SNAP_GET_TT(v)) {
	case VT::Number: return "number";
	case VT::Bool: return "boolean";
	case VT::Object: {
		const Obj* obj = SNAP_AS_OBJECT(v);
		if (SNAP_IS_STRING(v)) return "string";
		if (obj->tag == OT::func) return "function";
		return "object";
	}
	case VT::Nil: return "nil";
	case VT::Empty: return "empty";
	default: return "unknown";
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
