#pragma once
#include <vm.hpp>

namespace snap {
namespace stdlib {

int print(VM& vm, int argc);
int require(VM& vm, int argc);

}
}; // namespace snap