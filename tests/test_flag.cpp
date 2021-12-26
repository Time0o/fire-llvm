#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

void fire_main_flag(bool flag_a, bool flag_b)
{
  std::cout << "flag_a = " << flag_a << ", flag_b = " << flag_b << std::endl;
}

}

int main()
{
  fire::fire_llvm(fire_main_flag);
}
