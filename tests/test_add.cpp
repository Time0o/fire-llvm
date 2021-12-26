#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

int fire_main_add(int a, int b)
{
  return a + b;
}

}

int main()
{
  fire::fire_llvm(fire_main_add);
}
