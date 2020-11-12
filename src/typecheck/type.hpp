#pragma once
#include "../common.hpp"
#include <string>

namespace snap {

struct Type {
	const std::string name;
	u64 id;
	Type() = delete;
	Type(std::string s) : id{next_id++} {};
	std::string to_string() const;
	~Type();

	static bool is_numeric(const Type* t);
	static bool is_integer_type(const Type* t);
	static bool is_floating_type(const Type* t);

  private:
	static u64 next_id;
};

struct BuiltinType {
	static Type t_int;
	static Type t_float;
	static Type t_bool;
	static Type t_error;
	static Type t_string;
};

} // namespace snap