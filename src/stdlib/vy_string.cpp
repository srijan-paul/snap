#include "../str_format.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdlib/vy_string.hpp>
#include <util/args.hpp>
#include <util/lib_util.hpp>
#include <value.hpp>
#include <vm.hpp>

#define CHECK_ARG_TYPE(n, type)                                                                    \
	if (!check_arg_type(vm, n, type, fname)) return VYSE_NIL;

namespace vy::stdlib::primitives {

using namespace util;

#define FMT(...) kt::format_str(__VA_ARGS__)

/// @brief finds all occurrences of [find] in [src]
std::vector<size_t> find_ocurrences(const char* src, size_t srclen, const char* find,
									size_t findlen) {
	std::vector<size_t> indices;
	for (uint i = 0; i < srclen;) {
		bool match = true;
		uint j;
		for (j = 0; j < findlen; ++j) {
			if (src[i + j] != find[j]) {
				match = false;
				break;
			}
		}

		if (match) {
			indices.push_back(i);
			i += j;
		} else {
			++i;
		}
	}

	return indices;
}

Value substr(VM& vm, int argc) {
	static constexpr const char* fname = "String.substr";

	if (argc < 2 or argc > 3) {
		cfn_error(vm, fname,
				  FMT("Expected 2-3 arguments for call to String:substr (found {})", argc));
		return VYSE_NIL;
	}

	if (!check_arg_type(vm, 0, ObjType::string, fname)) return VYSE_NIL;
	if (!check_arg_type(vm, 1, ValueType::Number, fname)) return VYSE_NIL;

	const Value& vstring = vm.get_arg(0);
	const Value& vfrom = vm.get_arg(1);

	const String* str = VYSE_AS_STRING(vstring);

	number from_arg = VYSE_AS_NUM(vfrom);
	size_t from = from_arg;
	if (from_arg < 0 or from_arg != u64(from_arg)) {
		cfn_error(vm, fname, "argument #2 ('from_index') must be a non-negative whole number");
		return VYSE_NIL;
	}

	number len_arg = str->len() - from;
	size_t len = len_arg;
	if (argc == 3) {
		if (!check_arg_type(vm, 2, ValueType::Number, fname)) return VYSE_NIL;
		len_arg = VYSE_AS_NUM(vm.get_arg(2));

		if (len_arg < 0 or len_arg != u64(len_arg)) {
			cfn_error(vm, fname,
					  "argument #3 ('substr_length') must be a non-negative whole number");
			return VYSE_NIL;
		}

		len = len_arg;
	}

	if (from + len > str->len()) {
		cfn_error(vm, fname, "substring index out of bounds");
		return VYSE_NIL;
	}

	char* buf = new char[len + 1];
	for (size_t i = 0; i < len; ++i) {
		buf[i] = str->at(from + i);
	}
	buf[len] = '\0';

	String* sub = &vm.take_string(buf, len);
	return VYSE_OBJECT(sub);
}

Value code_at(VM& vm, int argc) {
	static constexpr const char* fname = "String.byte";

	if (argc < 2) {
		cfn_error(vm, fname, "too few arguments. expected at least 2 (string, index).");
		return VYSE_NIL;
	}

	/// check the first argument.
	const Value& strval = vm.get_arg(0);
	if (!check_arg_type(vm, 0, ObjType::string, fname)) return VYSE_NIL;
	const String& string = *VYSE_AS_STRING(strval);
	if (string.len() == 0) {
		cfn_error(vm, fname, "argument [string] must be at least one character long.");
		return VYSE_NIL;
	}

	/// check the second argument.
	const Value& v_index = vm.get_arg(1);
	if (!check_arg_type(vm, 1, ValueType::Number, fname)) return VYSE_NIL;

	number index = VYSE_AS_NUM(v_index);
	if (index < 0 or index >= string.len()) {
		cfn_error(vm, fname, "string index out of bounds.");
		return VYSE_NIL;
	}

	if (index != u64(index)) {
		cfn_error(vm, fname, "index must be a non-negative whole number.");
		return VYSE_NIL;
	}

	const char c = string.at(index);
	return VYSE_NUM(c);
}

/// @brief convert a vyse string to a double. Returns 'nan' if it can't be parsed as a double.
static number str2num_base10(const String& str) {
	const char* cstr = str.c_str();
	bool seen_period = false;

	for (const char* ch = cstr; *ch; ++ch) {
		if (!isdigit(*ch)) {
			if (*ch == '.') {
				if (seen_period) return NAN;
				seen_period = true;
			} else {
				return NAN;
			}
		}
	}
	/// TODO: remove this call to std::stod and do the parsing
	/// in the loop above.
	return std::stod(cstr);
}

/// TODO: Handle different bases from 2 to 64
static Value to_number(VM& vm, int argc) {
	util::Args args{vm, "String.to_num", 1, argc};
	const String& s = args.next<String>();
	return VYSE_NUM(str2num_base10(s));
}

static Value replace(VM& vm, int argc) {
	static constexpr const char* fname = "replace";

	Args args(vm, fname, 3, argc);
	const String& src = args.next<String>();
	const String& to_replace = args.next<String>();
	const String& replace_with = args.next<String>();

	const char* const str = src.c_str();
	const char* const find = to_replace.c_str();
	const char* const replace = replace_with.c_str();

	const size_t str_len = src.len();
	const size_t find_len = to_replace.len();
	const size_t replace_len = replace_with.len();

	/// A list of indices where [find] occurs in [str]
	const std::vector<size_t> indices = find_ocurrences(str, str_len, find, find_len);
	/// length of the final string after replacement.
	const size_t bufsize = str_len + (indices.size() * (int(replace_len) - int(find_len)));
	char* const buf = new char[bufsize + 1];
	buf[bufsize] = '\0';

	// current position in the source and destination buffers.
	uint src_pos = 0, dst_pos = 0;
	uint next_replace_idx = 0; // index of the next replacement pos
	while (src_pos < str_len) {
		// break out of the loop if there are no more replacements to be made.
		if (next_replace_idx >= indices.size()) break;
		if (indices[next_replace_idx] == src_pos) {
			// We've reached a location where [find] exists and needs to be replaced with [replace].
			std::memcpy(buf + dst_pos, replace, replace_len);
			dst_pos += replace_len;
			src_pos += find_len;
			++next_replace_idx;
		} else {
			buf[dst_pos++] = src[src_pos++];
		}
	}

	std::memcpy(buf + dst_pos, str + src_pos, str_len - src_pos);
	return VYSE_OBJECT(&vm.take_string(buf, bufsize));
}

/// @brief create a single character string from it's char code.
static Value from_code(VM& vm, int argc) {
	constexpr const char* fname = "String.from_code";
	Args args(vm, fname, 1, argc);

	s64 ascii = args.next_number();
	if (ascii < 0 or ascii > 255) {
		cfn_error(vm, fname, "ASCII char code must be between 0 and 255");
		return VYSE_NIL;
	}

	const char c = static_cast<char>(ascii);
	return VYSE_OBJECT(&vm.make_string(&c, 1));
}

static Value byte(VM& vm, int argc) {
	Args args(vm, "byte", 2, argc);
	const String& string = args.next<String>();
	const number idx = args.next_number();
	args.check(idx >= 0 and idx < string.len() and idx == s64(idx), "Invalid index.");
	args.check(string.len() > 0, "String must be at least one character long.");
	const char c = string.at(idx);
	return VYSE_NUM(c);
}

#define DECLARE_STR_CHECK_FUN(fn_name)                                                             \
	Value fn_name(VM& vm, int argc) {                                                              \
		Args args(vm, #fn_name, 1, argc);                                                          \
		const String& string = args.next<String>();                                                \
		if (string.len() == 0) return VYSE_BOOL(false);                                            \
		for (size_t i = 0; i < string.len(); ++i) {                                                \
			if (not std::fn_name(string[i])) return VYSE_BOOL(false);                              \
		}                                                                                          \
		return VYSE_BOOL(true);                                                                    \
	}

DECLARE_STR_CHECK_FUN(islower);
DECLARE_STR_CHECK_FUN(isupper);
DECLARE_STR_CHECK_FUN(isalpha);
DECLARE_STR_CHECK_FUN(isdigit);
DECLARE_STR_CHECK_FUN(isalnum);

#undef DECLARE_STR_CHECK_FUN

static Value slice(VM& vm, int argc) {
	Args args(vm, "List.slice", 3, argc);

	const String& string = args.next<String>();
	const number from = args.next_number();
	const number to = args.next_number();

	if (to >= from) return VYSE_NIL;

	args.check(string.in_range(from), "Bad argument #2 (from). String index out of range.");
	args.check(string.in_range(to), "Bad argument #3 (to). String index out of range.");

	const size_t start = from, end = to;
	const size_t buflen = end - start + 1;
	auto const buf = new char[buflen];
	for (size_t i = start; i <= end; ++i) {
		buf[i - start] = string[i];
	}

	buf[buflen - 1] = '\0';
	return VYSE_OBJECT(&vm.take_string(buf, end - start + 1));
}

void load_string_proto(VM& vm) {
	Table& str_proto = *vm.prototypes.string;
	add_libfn(vm, str_proto, "substr", substr);
	add_libfn(vm, str_proto, "code_at", code_at);
	add_libfn(vm, str_proto, "to_num", to_number);
	add_libfn(vm, str_proto, "replace", replace);
	add_libfn(vm, str_proto, "from_code", from_code);
	add_libfn(vm, str_proto, "byte", byte);
	add_libfn(vm, str_proto, "islower", islower);
	add_libfn(vm, str_proto, "isupper", isupper);
	add_libfn(vm, str_proto, "isdigit", isdigit);
	add_libfn(vm, str_proto, "isalpha", isalpha);
	add_libfn(vm, str_proto, "isalnum", isalnum);
	add_libfn(vm, str_proto, "slice", slice);
}

} // namespace vy::stdlib::primitives
