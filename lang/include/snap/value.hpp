#pragma once
#include "block.hpp"
#include "forward.hpp"
#include "token.hpp"
#include <string>

namespace snap {

enum class ObjType : u8 {
	string,
	proto,
	func,
	upvalue,
	table,
};

/// Objects always live on the heap. A value which is an object contains a pointer
/// to this data on the heap. The `tag` specifies what kind of object this is.
class Obj {
	// The VM and the Garbage Collector
	// need access to the mark bit and the
	// next pointer. So we'll declare them
	// as friend classes.
	friend VM;
	friend GC;
	friend Table;

public:
	const ObjType tag;

	explicit Obj(ObjType tt) noexcept : tag{tt} {};
	virtual const char* to_cstring() const;
	virtual ~Obj() = default;

protected:
	/// @brief pointer to the next object in the VM's GC linked list.
	Obj* next = nullptr;
	/// @brief Whether this object has been 'marked' as alive in the most
	/// currently active garbage collection cycle (if any).
	bool marked = false;

	/// @brief Traces all the references that this object
	/// contains to other values. Is overriden by deriving
	/// class.
	virtual void trace(GC& gc) = 0;

	/// @brief returns the size of this object in bytes.
	virtual size_t size() const = 0;
};

enum class ValueType { Number, Bool, Object, Nil, Empty };

// Without NaN tagging, values are represented
// as structs weighing 16 bytes. 1 word for the
// type tag and one for the union representing the
// possible states. This is a bit wasteful but
// not that bad.
/// TODO: Implement optional NaN tagging when a macro
/// SNAP_NAN_TAGGING is defined.
struct Value {
	ValueType tag;
	union {
		number num;
		bool boolean;
		Obj* object;
	} as;

	Value(number v) : tag{ValueType::Number} {
		as.num = v;
	}

	Value(bool v) : tag{ValueType::Bool} {
		as.boolean = v;
	}

	Value() : tag{ValueType::Nil} {};

	Value(Obj* o) : tag{ValueType::Object} {
		SNAP_ASSERT(o != nullptr, "Unexpected nullptr object");
		as.object = o;
	}

	inline number as_num() const {
		SNAP_ASSERT(is_num(), "Not a number.");
		return as.num;
	}

	inline bool as_bool() const {
		return as.boolean;
	}

	inline Obj* as_object() const {
		SNAP_ASSERT(is_object(), "Not a object.");
		return as.object;
	}

	inline bool is_bool() const {
		return tag == ValueType::Bool;
	}

	inline bool is_num() const {
		return tag == ValueType::Number;
	}

	inline bool is_object() const {
		return tag == ValueType::Object;
	}

	inline bool is_string() const {
		return tag == ValueType::Object && as_object()->tag == ObjType::string;
	}
};

bool operator==(const Value& a, const Value& b);
bool operator!=(const Value& a, const Value& b);

// It might seem redundant to represent
// these procedures as free functions instead
// of methods, but once we have NaN tagging, we
// would still like to have the same procedure
// signatures.
std::string value_to_string(Value v);
const char* value_type_name(Value v);
void print_value(Value v);

#define SNAP_SET_NUM(v, i)		((v).as.num = i)
#define SNAP_SET_BOOL(v, b)		((v).as.boolean = b)
#define SNAP_SET_OBJECT(v, o) ((v).as.object = o)

#define SNAP_NUM_VAL(n)		 (snap::Value(static_cast<snap::number>(n)))
#define SNAP_BOOL_VAL(b)	 (snap::Value(static_cast<bool>(b)))
#define SNAP_OBJECT_VAL(o) (snap::Value(static_cast<snap::Obj*>(o)))
#define SNAP_NIL_VAL			 (snap::Value())

#define SNAP_SET_TT(v, tt)		((v).tag = tt)
#define SNAP_GET_TT(v)				((v).tag)
#define SNAP_CHECK_TT(v, tt)	((v).tag == tt)
#define SNAP_ASSERT_TT(v, tt) (SNAP_ASSERT(SNAP_CHECK_TT((v), tt), "Mismatched type tags."))
#define SNAP_ASSERT_OT(v, ot)                                                                      \
	(SNAP_ASSERT((SNAP_AS_OBJECT(v)->tag == ot), "Mismatched type tags for objects"))
#define SNAP_TYPE_CSTR(v) (value_type_name(v))

#define SNAP_IS_NUM(v)		((v).tag == snap::ValueType::Number)
#define SNAP_IS_BOOL(v)		((v).tag == snap::ValueType::Bool)
#define SNAP_IS_NIL(v)		((v).tag == snap::ValueType::Nil)
#define SNAP_IS_EMPTY(v)	((v).tag == snap::ValueType::Empty)
#define SNAP_IS_OBJECT(v) ((v).tag == snap::ValueType::Object)
#define SNAP_IS_STRING(v) (SNAP_IS_OBJECT(v) and SNAP_AS_OBJECT(v)->tag == snap::ObjType::string)
#define SNAP_IS_TABLE(v)	(SNAP_IS_OBJECT(v) and SNAP_AS_OBJECT(v)->tag == snap::ObjType::table)

#define SNAP_AS_NUM(v)		(SNAP_ASSERT_TT((v), snap::ValueType::Number), (v).as.num)
#define SNAP_AS_BOOL(v)		(SNAP_ASSERT_TT((v), snap::ValueType::Bool), (v).as.boolean)
#define SNAP_AS_NIL(v)		(SNAP_ASSERT_TT((v), snap::ValueType::Nil), (v).as.double)
#define SNAP_AS_OBJECT(v) (SNAP_ASSERT_TT((v), snap::ValueType::Object), (v).as.object)
#define SNAP_AS_FUNCTION(v)                                                                        \
	(SNAP_ASSERT_OT((v), snap::ObjType::func), static_cast<snap::Function*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_PROTO(v)                                                                           \
	(SNAP_ASSERT_OT((v), snap::ObjType::proto), static_cast<snap::Prototype*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_STRING(v)                                                                          \
	(SNAP_ASSERT_OT((v), snap::ObjType::string), static_cast<snap::String*>(SNAP_AS_OBJECT(v)))
#define SNAP_AS_CSTRING(v) (SNAP_AS_STRING(v)->c_str())
#define SNAP_AS_TABLE(v)                                                                           \
	(SNAP_ASSERT_OT((v), snap::ObjType::table), static_cast<Table*>(SNAP_AS_OBJECT(v)))

#define SNAP_CAST_INT(v) (s64(SNAP_AS_NUM(v)))

} // namespace snap