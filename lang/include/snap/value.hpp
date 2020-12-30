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
	Float,
	Int,
	Bool,
	Object,
	Nil,
};

struct Value {
	ValueType tag;
	union {
		double float_;
		s64 int_;
		bool bool_;
		Obj* object;
	} as;

	Value(s64 v) : tag{ValueType::Int} {
		as.int_ = v;
	};
	Value(double v) : tag{ValueType::Float} {
		as.float_ = v;
	};
	Value(bool v) : tag{ValueType::Bool} {
		as.bool_ = v;
	};
	Value() : tag{ValueType::Nil} {
		as.float_ = 0.0f;
	};
	// string object value constructor
	Value(Obj* s) : tag{ValueType::Object} {};
	Value(char* s, int len);

	inline double as_float() const {
		return as.float_;
	}

	inline int as_int() const {
		return as.int_;
	}

	inline bool as_bool() const {
		return as.bool_;
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

	inline bool is_int() const {
		return tag == ValueType::Int;
	}

	inline bool is_float() const {
		return tag == ValueType::Float;
	}

	inline bool is_numeric() const {
		return (tag == ValueType::Float || tag == ValueType::Int);
	}

	inline bool is_object() const {
		return (tag == ValueType::Object);
	}

	inline bool is_string() const {
		return (tag == ValueType::Object && as_object()->tag == ObjType::string);
	}

	inline void set_float(float v) {
		tag = ValueType::Float;
		as.float_ = v;
	}

	inline void set_int(int i) {
		tag = ValueType::Int;
		as.int_ = i;
	}

	std::string name_str() const;
	const char* name_cstr() const;

	static bool are_equal(Value a, Value b);
	static ValueType numeric_upcast(Value& a, Value& b);
};

#define SNAP_INT_VAL(n)	   (snap::Value((s64)n))
#define SNAP_FLOAT_VAL(n)  (snap::Value((double)n))
#define SNAP_BOOL_VAL(b)   (snap::Value((bool)n))
#define SNAP_NIL_VAL	   (snap::Value())
#define SNAP_OBJECT_VAL(o) (snap::Value(static_cast<Obj*>(o)))

#define SNAP_IS_INT(v)	  ((v).tag == snap::ValueType::Int)
#define SNAP_IS_FLOAT(v)  ((v).tag == snap::ValueType::Float)
#define SNAP_IS_BOOL(v)	  ((v).tag == snap::ValueType::Bool)
#define SNAP_IS_NIL(v)	  ((v).tag == snap::ValueType::Nil)
#define SNAP_IS_OBJECT(v) ((v).tag == snap::ValueType::Object)
#define SNAP_IS_STRING(v) (SNAP_IS_OBJECT(v) && SNAP_AS_OBJECT(v)->tag == snap::ObjType::string)

#define SNAP_AS_INT(v)	   ((v).as.int_)
#define SNAP_AS_FLOAT(v)   ((v).as.float_)
#define SNAP_AS_BOOL(v)	   ((v).as.bool_)
#define SNAP_AS_NIL(v)	   ((v).as.bool_)
#define SNAP_AS_OBJECT(v)  ((v).as.object)
#define SNAP_AS_STRING(v)  (static_cast<String*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_CSTRING(v) ((SNAP_AS_STRING(v))->chars)

void print_value(Value v);
}; // namespace snap