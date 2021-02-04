#pragma once
#include "block.hpp"
#include "token.hpp"
#include <string>

namespace snap {

enum class ObjType : u8 { string, proto, func, upvalue, table };

using StackId = Value*;

// Objects always live on the heap. A value which is an object contains a a pointer
// to this data on the heap. The `tag` specifies what kind of object this is.
struct Obj {
	const ObjType tag;

	// pointer to the next object in the VM's GC linked list.
	Obj* next = nullptr;
	s32 m_hash = -1;
	bool marked = false;
	Obj(ObjType tt) : tag{tt} {};

	s32 hash();

	virtual ~Obj() = default;
};

enum class ValueType {
	Number,
	Bool,
	Object,
	Nil,
};

struct Value {
	ValueType tag;
	union {
		double num;
		bool boolean;
		Obj* object;
	} as;

	Value(double v) : tag{ValueType::Number} {
		as.num = v;
	}

	Value(bool v) : tag{ValueType::Bool} {
		as.boolean = v;
	}

	Value() : tag{ValueType::Nil} {};

	Value(Obj* o) : tag{ValueType::Object} {
		as.object = o;
	}

	inline number as_num() const {
		return as.num;
	}

	inline bool as_bool() const {
		return as.boolean;
	}

	inline Obj* as_object() const {
		return as.object;
	}

	inline bool is_bool() const {
		return tag == ValueType::Bool;
	}

	inline bool is_num() const {
		return tag == ValueType::Number;
	}

	inline bool is_object() const {
		return (tag == ValueType::Object);
	}

	inline bool is_string() const {
		return (tag == ValueType::Object && as_object()->tag == ObjType::string);
	}

	std::string name_str() const;
	const char* name_cstr() const;
	const char* type_name() const;
};

bool operator==(const Value& a, const Value& b);
bool operator!=(const Value& a, const Value& b);

#define SNAP_SET_NUM(v, i)	  ((v).as.num = i)
#define SNAP_SET_BOOL(v, b)	  ((v).as.boolean = b)
#define SNAP_SET_OBJECT(v, o) ((v).as.object = o)

#define SNAP_NUM_VAL(n)		 (snap::Value(static_cast<number>(n)))
#define SNAP_BOOL_VAL(b)	 (snap::Value(static_cast<bool>(b)))
#define SNAP_CONST_OBJECT(o) (snap::Value(static_cast<const Obj*>(o)))
#define SNAP_OBJECT_VAL(o)	 (snap::Value(static_cast<Obj*>(o)))
#define SNAP_NIL_VAL		 (snap::Value())

#define SNAP_IS_NUM(v)	  ((v).tag == snap::ValueType::Number)
#define SNAP_IS_BOOL(v)	  ((v).tag == snap::ValueType::Bool)
#define SNAP_IS_NIL(v)	  ((v).tag == snap::ValueType::Nil)
#define SNAP_IS_OBJECT(v) ((v).tag == snap::ValueType::Object)
#define SNAP_IS_STRING(v) (SNAP_IS_OBJECT(v) && SNAP_AS_OBJECT(v)->tag == snap::ObjType::string)

#define SNAP_AS_NUM(v)		((v).as.num)
#define SNAP_AS_BOOL(v)		((v).as.boolean)
#define SNAP_AS_NIL(v)		((v).as.double)
#define SNAP_AS_OBJECT(v)	((v).as.object)
#define SNAP_AS_FUNCTION(v) (static_cast<snap::Function*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_STRING(v)	(static_cast<snap::String*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_CSTRING(v)	((SNAP_AS_STRING(v))->c_str())
#define SNAP_AS_TABLE(v)	(static_cast<Table*>(SNAP_AS_OBJECT(v)))

#define SNAP_SET_TT(v, tt) ((v).tag = tt)
#define SNAP_GET_TT(v)	   ((v).tag)
#define SNAP_TYPE_CSTR(v)  ((v).type_name())

#define SNAP_CAST_INT(v) ((s64)((v).as.num))

void print_value(Value v);
} // namespace snap