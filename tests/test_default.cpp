#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

int fire_main_default(int def = 0)
{
  std::cout << "def = " << def << std::endl;
  return 0;
}

}

int main()
{
  fire::fire_llvm(fire_main_default);
}
