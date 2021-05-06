#pragma once
#include "block.hpp"
#include "forward.hpp"
#include "token.hpp"
#include <string>

namespace vyse {

enum class ObjType : u8 {
	string,
	codeblock,
	closure,
	c_closure,
	upvalue,
	table,
	user_data,
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
	Obj(Obj&& o) = default;
	Obj(Obj const& o) = default;
	virtual ~Obj() = default;

	virtual const char* to_cstring() const;

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

enum class ValueType { Number, Bool, Object, Nil, Undefined };

// Without NaN tagging, values are represented
// as structs weighing 16 bytes. 1 word for the
// type tag and one for the union representing the
// possible states. This is a bit wasteful but
// not that bad.
/// TODO: Implement optional NaN tagging when a macro
/// VYSE_NAN_TAGGING is defined.
struct Value {
	ValueType tag;
	union Data {
		number num;
		bool boolean;
		Obj* object;
		Data(){};
		Data(number v) noexcept : num(v){};
		Data(bool b) noexcept : boolean(b){};
		Data(Obj* o) noexcept : object(o){};
	} as;

	explicit Value(number n) noexcept : tag{ValueType::Number}, as{n} {};
	explicit Value(bool b) noexcept : tag{ValueType::Bool}, as{b} {};
	explicit Value() noexcept : tag{ValueType::Nil} {};
	explicit Value(Obj* o) noexcept : tag{ValueType::Object}, as{o} {
		VYSE_ASSERT(o != nullptr, "Unexpected nullptr object");
	}

	static inline Value undefined() {
		Value undef;
		undef.tag = ValueType::Undefined;
		return undef;
	}
};

bool operator==(const Value& a, const Value& b);
bool operator!=(const Value& a, const Value& b);

// It might seem redundant to represent
// these procedures as free functions instead
// of methods, but once we have NaN tagging, we
// would still like to have the same procedure
// signatures used across the codebase.
std::string value_to_string(Value v);
char* value_to_cstring(Value v);
const char* value_type_name(Value v);
void print_value(Value v);

#define VYSE_SET_NUM(v, i)		((v).as.num = i)
#define VYSE_SET_BOOL(v, b)		((v).as.boolean = b)
#define VYSE_SET_OBJECT(v, o) ((v).as.object = o)

#define VYSE_NUM(n)		 (vyse::Value(static_cast<vyse::number>(n)))
#define VYSE_BOOL(b)	 (vyse::Value(static_cast<bool>(b)))
#define VYSE_OBJECT(o) (vyse::Value(static_cast<vyse::Obj*>(o)))
#define VYSE_NIL			 (vyse::Value())
#define VYSE_UNDEF		 (vyse::Value::undefined())

#define VYSE_SET_TT(v, tt)		((v).tag = tt)
#define VYSE_GET_TT(v)				((v).tag)
#define VYSE_CHECK_TT(v, tt)	((v).tag == tt)
#define VYSE_ASSERT_TT(v, tt) (VYSE_ASSERT(VYSE_CHECK_TT((v), tt), "Mismatched type tags."))
#define VYSE_ASSERT_OT(v, ot)                                                                      \
	(VYSE_ASSERT((VYSE_AS_OBJECT(v)->tag == ot), "Mismatched object types."))
#define VYSE_TYPE_CSTR(v) (value_type_name(v))

#define VYSE_IS_NUM(v)			 ((v).tag == vyse::ValueType::Number)
#define VYSE_IS_BOOL(v)			 ((v).tag == vyse::ValueType::Bool)
#define VYSE_IS_NIL(v)			 ((v).tag == vyse::ValueType::Nil)
#define VYSE_IS_UNDEFINED(v) ((v).tag == vyse::ValueType::Undefined)
#define VYSE_IS_OBJECT(v)		 ((v).tag == vyse::ValueType::Object)
#define VYSE_IS_STRING(v)		 (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vyse::ObjType::string)
#define VYSE_IS_TABLE(v)		 (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vyse::ObjType::table)

#define VYSE_AS_NUM(v)			((v).as.num)
#define VYSE_AS_BOOL(v)			((v).as.boolean)
#define VYSE_AS_NIL(v)			((v).as.double)
#define VYSE_AS_OBJECT(v)		((v).as.object)
#define VYSE_AS_CLOSURE(v)	(static_cast<vyse::Closure*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_CCLOSURE(v) (static_cast<vyse::CClosure*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_PROTO(v)		(static_cast<vyse::CodeBlock*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_STRING(v)		(static_cast<vyse::String*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_CSTRING(v)	(VYSE_AS_STRING(v)->c_str())
#define VYSE_AS_TABLE(v)		(static_cast<Table*>(VYSE_AS_OBJECT(v)))

#define VYSE_CAST_INT(v) (s64(VYSE_AS_NUM(v)))

} // namespace vyse