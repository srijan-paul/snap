// KT header-only library
// Requirements: C++17

#pragma once
#include <ostream>
#include <sstream>
#include <string_view>

namespace kt {
inline constexpr std::string_view fmt_token = "{}";

inline std::ostream& format_str(std::ostream& out, std::string_view fmt) {
	out << fmt;
	return out;
}

template <typename Arg, typename... Args>
std::ostream& format_str(std::ostream& out, std::string_view fmt, Arg&& arg, Args&&... args) {
	if (auto search = fmt.find(fmt_token); search != std::string::npos) {
		std::string_view text(fmt.data(), search);
		out << text << std::forward<Arg>(arg);
		return format_str(out, fmt.substr(search + fmt_token.size()), std::forward<Args>(args)...);
	}
	return format_str(out, fmt);
}

template <typename... Args>
std::string format_str(std::string_view fmt, Args&&... args) {
	std::stringstream str;
	format_str(str, fmt, std::forward<Args>(args)...);
	return str.str();
}
} // namespace kt
