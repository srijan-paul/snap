#pragma once
#include "block.hpp"
#include "common.hpp"
#include "token.hpp"
#include <cstdint>
#include <string>

namespace snap {

enum class ObjType : u8 { string, func };

using StackId = Value*;

class VM;

// Objects always live on the heap. A value which is an object contains a a pointer
// to this data on the heap. The `tag` specifies what kind of object this is.
struct Obj {
	ObjType tag;

	// pointer to the next object in the VM's GC linked list.
	Obj* next = nullptr;
	s32 m_hash = -1;
	bool marked = false;
	Obj(ObjType tt) : tag{tt} {};
	Obj(VM& vm, ObjType tt);

	s32 hash();
};

/// Strings in snap are heap allocated, and contain 3 important fields:
/// `chars`  -> Pointer to the first character of the string on the heap (null terminated).
/// `length` -> Length of the string.
struct String : Obj {
	const char* chars = nullptr;
	size_t length = 0;
	/// @param chrs pointer to the character buffer. must be null terminated.
	/// @param len length of the string.
	String(const char* chrs, size_t len);
	// creates a string that owns the characters `chrs`.
	String(char* chrs) : Obj(ObjType::string), chars{chrs} {};
	String(VM& vm, const char* chrs, size_t len);

	s32 hash();

	static String* concatenate(const String* a, const String* b);
	~String();
};

// A protoype is the body of a function
// that contains the bytecode.
struct Prototype {
	String* name;
	u32 num_params = 0;
	Block m_block;
	u32 num_upvals = 0;
	Prototype(String* funcname) : name{funcname} {};
	Prototype(String* funcname, u32 param_count) : name{funcname}, num_params{param_count} {};
};

struct Function : Obj {
	Prototype* proto;
	Function(String* name) : Obj(ObjType::func), proto(new Prototype(name)){};
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

	Value() : tag{ValueType::Nil} {
		as.num = 0.0f;
	}

	Value(Obj* o) : tag{ValueType::Object} {
		as.object = o;
	}
	Value(char* s, int len); // string object value constructor

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

#define SNAP_AS_NUM(v)		((v).as.num)
#define SNAP_AS_BOOL(v)		((v).as.boolean)
#define SNAP_AS_NIL(v)		((v).as.double)
#define SNAP_AS_OBJECT(v)	((v).as.object)
#define SNAP_AS_FUNCTION(v) (static_cast<snap::Function*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_STRING(v)	(static_cast<snap::String*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_CSTRING(v)	((SNAP_AS_STRING(v))->chars)

#define SNAP_SET_TT(v, tt) ((v).tag = tt)
#define SNAP_GET_TT(v)	   ((v).tag)
#define SNAP_TYPE_CSTR(v)  ((v).type_name())

#define SNAP_CAST_INT(v) ((s64)((v).as.num))

void print_value(Value v);
} // namespace snap