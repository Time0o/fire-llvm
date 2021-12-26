#include <fire-llvm/fire.hpp>

#include <iostream>
#include <optional>

namespace {

// XXX make fire::optional implement complete std::optional interface
void fire_main_optional(std::optional<int> opt)
{
  if (opt)
    std::cout << "opt = " << opt.value() << std::endl;
  else
    std::cout << "opt = nothing" << std::endl;
}

}

int main()
{
  fire::fire_llvm(fire_main_optional);
}
