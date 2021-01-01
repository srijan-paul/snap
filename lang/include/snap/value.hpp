#pragma once
#include "common.hpp"
#include "token.hpp"
#include <cstdint>
#include <string>

namespace snap {

enum ObjType { string };

// Objects always live on the heap. A value which is an object contains a a pointer
// to this data on the heap. The `tag` specifies what kind of object this is.
struct Obj {
	ObjType tag;

	// pointer to the next object in the VM's GC linked list.
	Obj* next;
	size_t hash = -1;
	Obj(ObjType tt) : tag{tt} {};
};

/// Strings in snap are heap allocated, and contain 3 important fields:
/// `chars`  -> Pointer to the first character of the string on the heap (null terminated).
/// `length` -> Length of the string.
struct String : Obj {
	char* chars = nullptr;
	size_t length = 0;
	/// @param chrs pointer to the character buffer. must be null terminated.
	/// @param len length of the string.
	String(char* chrs, size_t len) : Obj(ObjType::string), chars{chrs}, length{len} {};

	static String* concatenate(const String* a, const String* b);
	~String();
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
	};
	Value(bool v) : tag{ValueType::Bool} {
		as.boolean = v;
	};
	Value() : tag{ValueType::Nil} {
		as.num = 0.0f;
	};
	// string object value constructor
	Value(Obj* s) : tag{ValueType::Object} {};
	Value(char* s, int len);

	inline number as_num() const {
		return as.num;
	}

	inline bool as_bool() const {
		return as.boolean;
	}

	inline Obj* as_object() const {
		return as.object;
	}

	inline String* as_string() const {
		return static_cast<String*>(as.object);
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

	static bool are_equal(Value a, Value b);
};

#define SNAP_SET_NUM(v, i)	  ((v).as.num = i)
#define SNAP_SET_BOOL(v, b)	  ((v).as.boolean = b)
#define SNAP_SET_OBJECT(v, o) ((v).as.object = o)

#define SNAP_NUM_VAL(n)	   (snap::Value(static_cast<number>(n)))
#define SNAP_BOOL_VAL(b)   (snap::Value(static_cast<bool>(b)))
#define SNAP_NIL_VAL	   (snap::Value())
#define SNAP_OBJECT_VAL(o) (snap::Value(static_cast<Obj*>(o)))

#define SNAP_IS_NUM(v)	  ((v).tag == snap::ValueType::Number)
#define SNAP_IS_BOOL(v)	  ((v).tag == snap::ValueType::Bool)
#define SNAP_IS_NIL(v)	  ((v).tag == snap::ValueType::Nil)
#define SNAP_IS_OBJECT(v) ((v).tag == snap::ValueType::Object)
#define SNAP_IS_STRING(v) (SNAP_IS_OBJECT(v) && SNAP_AS_OBJECT(v)->tag == snap::ObjType::string)

#define SNAP_AS_NUM(v)	   ((v).as.num)
#define SNAP_AS_BOOL(v)	   ((v).as.boolean)
#define SNAP_AS_NIL(v)	   ((v).as.double)
#define SNAP_AS_OBJECT(v)  ((v).as.object)
#define SNAP_AS_STRING(v)  (static_cast<String*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_CSTRING(v) ((SNAP_AS_STRING(v))->chars)

#define SNAP_SET_TT(v, tt) ((v).tag = tt)
#define SNAP_GET_TT(v)	   ((v).tag)
#define SNAP_TYPE_CSTR(v)  ((v).type_name())

#define SNAP_CAST_INT(v) ((s64)((v).as.num))

void print_value(Value v);
}; // namespace snap