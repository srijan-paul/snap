#pragma once
#include "../scanner.hpp"

namespace snap {

class Compiler {
  public:
	Compiler(const std::string* source) : source{source}, scanner{Scanner(source)} {};

  private:
	const std::string* source;
	const Scanner scanner;
};

} // namespace snap