#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

int fire_main_flag(bool flag_a, bool flag_b)
{
  std::cout << "flag_a = " << flag_a << ", flag_b = " << flag_b << std::endl;
  return 0;
}

}

int main()
{
  fire::fire_llvm(fire_main_flag);
}
