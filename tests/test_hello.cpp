#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

int fire_main_hello(std::string const &msg)
{
  std::cout << msg << std::endl;
  return 0;
}

}

int main()
{
  fire::fire_llvm(fire_main_hello);
}
