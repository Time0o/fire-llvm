#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

int fire_main_default(int def = 0)
{
  return def;
}

}

int main()
{
  fire::fire_llvm(fire_main_default);
}
