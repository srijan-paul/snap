#pragma once
#include "block.hpp"
#include "forward.hpp"
#include "token.hpp"
#include "value.hpp"
#include <cassert>
#include <string>

namespace vy {

enum class ObjType : u8 {
	string,
	codeblock,
	closure,
	c_closure,
	upvalue,
	table,
	list,
	user_data,
};

/// Objects always live on the heap. A value which is an object contains a pointer
/// to this data on the heap. The `tag` specifies what kind of object this is.
class Obj {
	// The VM and the Garbage Collector need access to the mark bit and the `next` pointer. So we'll
	// declare them as friend classes.
	friend VM;
	friend GC;
	friend Table;

  public:
	const ObjType tag;

	explicit constexpr Obj(ObjType tt) noexcept : tag{tt} {}
	constexpr Obj(Obj&& o) = default;
	constexpr Obj(Obj const& o) = default;

	virtual ~Obj() = default;

	virtual const char* to_cstring() const;

  protected:
	/// @brief pointer to the next object in the VM's GC linked list.
	Obj* next = nullptr;
	/// @brief Whether this object has been 'marked' as alive in the most
	/// currently active garbage collection cycle (if any).
	/// TODO: use the MSB in the `next` pointer for this task?
	bool marked = false;

	/// @brief Traces all the references that this object
	/// contains to other values. Is overriden by deriving
	/// class.
	virtual void trace(GC& gc) = 0;

	/// @brief returns the size of this object in bytes.
	virtual size_t size() const = 0;
};

enum class ValueType : u8 { Number, Bool, Object, Nil, Undefined, MiscData };

// Without NaN tagging, values are represented as structs weighing 16 bytes. 1 word for the type tag
// and one for the union representing the possible states. This is a bit wasteful but not that bad.

/// TODO: Implement optional NaN tagging when a macro
/// VYSE_NAN_TAGGING is defined.

struct Value {
	ValueType tag;
	union Data {
		number num;
		bool boolean;
		Obj* object;
		void* misc_data;
		constexpr Data() noexcept : num(0) {}
		constexpr Data(number v) noexcept : num(v) {}
		constexpr Data(bool b) noexcept : boolean(b) {}
		constexpr Data(Obj* o) noexcept : object(o) {}
		constexpr Data(void* p) noexcept : misc_data(p) {}
	} as;

	explicit constexpr Value(number n) noexcept : tag{ValueType::Number}, as{n} {}
	explicit constexpr Value(bool b) noexcept : tag{ValueType::Bool}, as{b} {}
	explicit constexpr Value() noexcept : tag{ValueType::Nil} {}
	explicit constexpr Value(void* p) noexcept : tag{ValueType::MiscData}, as{p} {}
	explicit constexpr Value(Obj* o) noexcept : tag{ValueType::Object}, as{o} {
		VYSE_ASSERT(o != nullptr, "Unexpected nullptr object");
	}

	static inline constexpr Value undefined() noexcept {
		Value undef;
		undef.tag = ValueType::Undefined;
		return undef;
	}
};

bool operator==(const Value& a, const Value& b);
bool operator!=(const Value& a, const Value& b);

// It might seem redundant to represent these procedures as free functions instead of methods, but
// once we have NaN tagging, we would still like to have the same procedure signatures used across
// the codebase.
const char* vtype_to_string(ValueType tag);
const char* otype_to_string(ObjType tag);
std::string value_to_string(Value v);
char* num_to_cstring(number n);
const char* value_type_name(Value v);
void print_value(Value v);

#define VYSE_SET_NUM(v, i) ((v).as.num = i)
#define VYSE_SET_BOOL(v, b) ((v).as.boolean = b)
#define VYSE_SET_OBJECT(v, o) ((v).as.object = o)

#define VYSE_NUM(n) (vy::Value(static_cast<vy::number>(n)))
#define VYSE_BOOL(b) (vy::Value(static_cast<bool>(b)))
#define VYSE_OBJECT(o) (vy::Value(static_cast<vy::Obj*>(o)))
#define VYSE_NIL (vy::Value())
#define VYSE_UNDEF (vy::Value::undefined())

#define VYSE_SET_TT(v, tt) ((v).tag = tt)
#define VYSE_GET_TT(v) ((v).tag)
#define VYSE_CHECK_TT(v, tt) ((v).tag == tt)
#define VYSE_ASSERT_TT(v, tt) (VYSE_ASSERT(VYSE_CHECK_TT((v), tt), "Mismatched type tags."))
#define VYSE_ASSERT_OT(v, ot)                                                                      \
	(VYSE_ASSERT((VYSE_AS_OBJECT(v)->tag == ot), "Mismatched object types."))
#define VYSE_TYPE_CSTR(v) (value_type_name(v))

#define VYSE_IS_NUM(v) ((v).tag == vy::ValueType::Number)
#define VYSE_IS_BOOL(v) ((v).tag == vy::ValueType::Bool)
#define VYSE_IS_FALSE(v) (VYSE_IS_BOOL(v) and !VYSE_AS_BOOL(v))
#define VYSE_IS_TRUE(v) (VYSE_IS_BOOL(v) and VYSE_AS_BOOL(v))
#define VYSE_IS_NIL(v) ((v).tag == vy::ValueType::Nil)
#define VYSE_IS_UNDEFINED(v) ((v).tag == vy::ValueType::Undefined)
#define VYSE_IS_OBJECT(v) ((v).tag == vy::ValueType::Object)

#define VYSE_IS_STRING(v) (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vy::ObjType::string)
#define VYSE_IS_TABLE(v) (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vy::ObjType::table)
#define VYSE_IS_LIST(v) (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vy::ObjType::list)
#define VYSE_IS_CLOSURE(v) (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vy::ObjType::closure)
#define VYSE_IS_CODEBLOCK(v)                                                                       \
	(VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vy::ObjType::codeblock)
#define VYSE_IS_CCLOSURE(v) (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vy::ObjType::c_closure)
#define VYSE_IS_UDATA(v) (VYSE_IS_OBJECT(v) and VYSE_AS_OBJECT(v)->tag == vy::ObjType::user_data)

#define VYSE_IS_FALSY(v) ((VYSE_IS_BOOL(v) and !(VYSE_AS_BOOL(v))) or VYSE_IS_NIL(v))
#define VYSE_IS_TRUTHY(v) (!VYSE_IS_FALSY(v))

#define VYSE_AS_NUM(v) ((v).as.num)
#define VYSE_AS_BOOL(v) ((v).as.boolean)
#define VYSE_AS_NIL(v) ((v).as.double)
#define VYSE_AS_OBJECT(v) ((v).as.object)
#define VYSE_AS_CLOSURE(v) (static_cast<vy::Closure*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_CCLOSURE(v) (static_cast<vy::CClosure*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_PROTO(v) (static_cast<vy::CodeBlock*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_STRING(v) (static_cast<vy::String*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_CSTRING(v) (VYSE_AS_STRING(v)->c_str())
#define VYSE_AS_TABLE(v) (static_cast<Table*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_LIST(v) (static_cast<List*>(VYSE_AS_OBJECT(v)))
#define VYSE_AS_UDATA(v) (static_cast<UserData*>(VYSE_AS_OBJECT(v)))

#define VYSE_CAST_INT(v) (s64(VYSE_AS_NUM(v)))

inline constexpr bool is_val_falsy(const Value& value) noexcept {
	return VYSE_IS_FALSE(value) or VYSE_IS_NIL(value);
}

inline constexpr bool is_val_truthy(const Value& value) noexcept {
	return !is_val_falsy(value);
}

inline constexpr bool is_integer(number num) noexcept {
	return num == s64(num);
}

template <typename T>
inline constexpr T value_get(Value const& value) {
	if constexpr (std::is_arithmetic_v<T>) {
		return VYSE_AS_NUM(value);
	} else if constexpr (std::is_same_v<T, bool>) {
		return VYSE_AS_BOOL(value);
	} else {
		static_assert(std::is_same_v<T, Obj*>, "Unsupported type.");
		return VYSE_AS_OBJECT(value);
	}
}

} // namespace vy
