#pragma once
#include "common.hpp"
#include "compiler/compiler.hpp"
#include "syntax/scanner.hpp"
#include "syntax/parser.hpp"
#include "vm/vm.hpp"

namespace snap {

struct Runtime {
	Runtime(std::string code);
  private:
	std::string m_name;
	std::string m_code;
	VM m_vm;
};

}
