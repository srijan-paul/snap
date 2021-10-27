#include <iostream>
#include <string>
#include <token.hpp>

namespace vyse {

std::string Token::raw(const std::string& source) const {
	auto pos = source_pos();
	return source.substr(pos.start, pos.length);
}

const char* Token::raw_cstr(const std::string& source) const {
	return source.c_str() + location.source_pos.start;
}

SourcePosition Token::source_pos() const {
	return location.source_pos;
}

} // namespace vyse

