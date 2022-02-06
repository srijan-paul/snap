#include "common.hpp"
#include <cassert>
#include <cstdio>
#include <list.hpp>
#include <vm.hpp>

namespace vy {

using VT = ValueType;
using OT = ObjType;

const char* Obj::to_cstring() const {
	return "[vyse object]";
}

void print_value(Value v) {
	std::printf("%s", value_to_string(v).c_str());
}

/// Convert a vyse number to a cstring.
char* num_to_cstring(number num) {
	// If a whole number, then truncate decimal part (.0000)
	bool is_whole = num == s64(num);

	int bufsize;
	if (is_whole) {
		bufsize = snprintf(nullptr, 0, "%lld", s64(num));
	} else {
		bufsize = snprintf(nullptr, 0, "%.7g", num);
	}

	char* buf = new char[bufsize + 1];
	buf[bufsize] = '\0';
	int res;
	if (is_whole) {
		res = sprintf(buf, "%lld", s64(num));
	} else {
		res = sprintf(buf, "%.7g", num);
	}

	VYSE_ASSERT(res > 0, "sprintf failed!");
	return buf;
}

std::string value_to_string(Value v) {
	switch (VYSE_GET_TT(v)) {
	case VT::Number: {
		number num = VYSE_AS_NUM(v);
		if (s64(num) == num) return std::to_string(s64(num));
		return std::to_string(num);
	}

	case VT::Bool: return VYSE_AS_BOOL(v) ? "true" : "false";
	case VT::Nil: return "nil";
	case VT::Undefined: return "undefined";
	case VT::Object: {
		const Obj* obj = VYSE_AS_OBJECT(v);

		switch (obj->tag) {
		case OT::string: return VYSE_AS_CSTRING(v);
		case OT::closure:
			return std::string("[fn ") + static_cast<const Closure*>(obj)->name_cstr() + "]";
		case OT::c_closure: return "[ native fn ]";
		case OT::codeblock:
			return std::string("[code ") + static_cast<const CodeBlock*>(obj)->name_cstr() + "]";
		case OT::upvalue: {
			const Upvalue* upval = static_cast<const Upvalue*>(obj);
			return value_to_string(*(upval->m_value));
		}
		case OT::table: {
			Table* tbl = VYSE_AS_TABLE(v);
			return "[table " + std::to_string((size_t)tbl) + "]";
		}
		case OT::list: {
			List* list = VYSE_AS_LIST(v);
			return "[list " + std::to_string((size_t)list) + "]";
		}

		default: return std::string(obj->to_cstring());
		}
	}
	default: VYSE_ERROR("Impossible value tag."); break;
	}
	return "<unknown>";
}

const char* vtype_to_string(VT tag) {
	switch (tag) {
	case VT::Number: return "number";
	case VT::Bool: return "boolean";
	case VT::Object: return "object";
	case VT::Nil: return "nil";
	case VT::Undefined: return "undefined";
	default: return "unknown";
	}
}

const char* otype_to_string(ObjType tag) {
	switch (tag) {
	case OT::string: return "string";
	case OT::table: return "table";
	case OT::closure: return "function";
	case OT::c_closure: return "native function";
	case OT::list: return "list";
	case OT::user_data: return "userdata";
	default: return "unknown";
	}
}

const char* value_type_name(Value v) {
	VT tag = VYSE_GET_TT(v);
	VYSE_ASSERT(tag >= VT::Number and tag <= VT::MiscData, "Impossible type tag.");

	if (VYSE_IS_OBJECT(v)) {
		return otype_to_string(VYSE_AS_OBJECT(v)->tag);
	}

	return vtype_to_string(tag);
}

bool operator==(const Value& a, const Value& b) {
	if (a.tag != b.tag) return false;
	switch (a.tag) {
	case VT::Number: return VYSE_AS_NUM(b) == VYSE_AS_NUM(a);
	case VT::Bool: return VYSE_AS_BOOL(a) == VYSE_AS_BOOL(b);
	case VT::Object: {
		const Obj* oa = VYSE_AS_OBJECT(a);
		const Obj* ob = VYSE_AS_OBJECT(b);
		if (oa->tag != ob->tag) return false;
		return oa == ob;
	}
	case VT::Nil: return true;
	default: return false;
	}
}

bool operator!=(const Value& a, const Value& b) {
	return !(a == b);
}

} // namespace vy
