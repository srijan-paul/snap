#include "token.hpp"
#include <iostream>
#include <string>

namespace snap {

std::string Token::raw(const std::string& source) const {
	auto pos = source_pos();
	return source.substr(pos.start, pos.end);
}

SourcePosition Token::source_pos() const {
	return location.source_pos;
}

} // namespace snap