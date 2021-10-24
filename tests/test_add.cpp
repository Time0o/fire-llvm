#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

int fire_main_add(int x, int y)
{
  std::cout << x << " + " << y << " = " << x + y << std::endl;
  return 0;
}

}

int main()
{
  fire::fire_llvm(fire_main_add);
}
