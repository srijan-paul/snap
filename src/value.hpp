#include "common.hpp"
#include <stdint.h>

namespace snap {
// using Value = double;

enum class ValueType {
	Float,
	String,
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
	} as;

	Value(s64 v) : tag{ValueType::Int}, as{.int_ = v} {};
	Value(double v) : tag{ValueType::Float}, as{.float_ = v} {};
	Value(bool v) : tag{ValueType::Bool}, as{.bool_ = v} {};
	Value() : tag{ValueType::Nil}, as{.float_ = 0} {};

	inline double as_float() {
		return as.float_;
	}

	inline int as_int() {
		return as.int_;
	}

	inline bool as_bool() {
		return as.bool_;
	}

	inline bool is_bool() {
		return tag == ValueType::Bool;
	}

	inline bool is_int() {
		return tag == ValueType::Int;
	}

	inline bool is_float() {
		return tag == ValueType::Float;
	}

	inline bool is_numeric() {
		return (tag == ValueType::Float || tag == ValueType::Int);
	}

	inline void set_float(float v) {
		tag = ValueType::Float;
		as.float_ = v;
	}

	inline void set_int(int i) {
		tag = ValueType::Int;
		as.int_ = i;
	}

	static bool are_equal(Value a, Value b);
	static ValueType numeric_upcast(Value& a, Value& b);
};

#define SNAP_INT_VAL(n)	  (snap::Value((s64)n))
#define SNAP_FLOAT_VAL(n) (snap::Value((double)n))
#define SNAP_BOOL_VAL(b)  (snap::Value((bool)n))
#define SNAP_NIL_VAL	  (snap::Value())

#define SNAP_IS_INT(v)	 ((v).tag == ValueType::Int)
#define SNAP_IS_FLOAT(v) ((v).tag == ValueType::Float)
#define SNAP_IS_BOOL(v)	 ((v).tag == ValueType::Bool)

#define SNAP_AS_INT(v)	 ((v).as.int_)
#define SNAP_AS_FLOAT(v) ((v).as.float_)
#define SNAP_AS_BOOL(v)	 ((v).as.bool_)
#define SNAP_AS_NIL(v)	 ((v).as.bool_)

void print_value(Value v);
bool values_equal(Value a, Value b);
}; // namespace snap