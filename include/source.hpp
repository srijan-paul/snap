#pragma once
#include <optional>
#include <string>

namespace vy {

struct SourceCode {
	static std::optional<SourceCode> from_path(std::string file_path);

	std::string path;
	std::string code;
};

}; // namespace vy