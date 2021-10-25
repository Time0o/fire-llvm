#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

int fire_main_add(int a, int b)
{
  std::cout << "a + b = " << a + b << std::endl;
  return 0;
}

}

int main()
{
  fire::fire_llvm(fire_main_add);
}
