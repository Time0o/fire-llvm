#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

void fire_main_hello(std::string const &msg)
{
  std::cout << msg << std::endl;
}

}

int main()
{
  fire::fire_llvm(fire_main_hello);
}
