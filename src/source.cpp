#include <filesystem>
#include <fstream>
#include <source.hpp>
#include <sstream>

namespace vy {

std::optional<SourceCode> SourceCode::from_path(std::string path) {
	std::filesystem::path fpath{std::move(path)};
	if (!fpath.is_absolute()) {
		fpath = std::filesystem::absolute(std::move(fpath));
	}

	if (!fpath.is_absolute() || !std::filesystem::is_regular_file(fpath) || fpath.empty()) {
		return std::nullopt;
	}

	std::ifstream stream{fpath};
	if (!stream) return std::nullopt;

	std::stringstream contents;
	contents << stream.rdbuf();

	return SourceCode{fpath.string(), contents.str()};
}

} // namespace vy
