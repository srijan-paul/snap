#include "type.hpp"

namespace snap {

u64 Type::next_id = 0;

Type BuiltinType::t_int("int");
Type BuiltinType::t_float("float");
Type BuiltinType::t_bool("bool");
Type BuiltinType::t_error("<error>");
Type BuiltinType::t_string("string");

bool Type::is_numeric(const Type* t) {
	return t == &BuiltinType::t_int || t == &BuiltinType::t_float;
}

bool Type::is_integer_type(const Type* t) {
	return t == &BuiltinType::t_int;
}

bool Type::is_floating_type(const Type* t) {
	return t == &BuiltinType::t_float;
}

Type::~Type(){};

} // namespace snap