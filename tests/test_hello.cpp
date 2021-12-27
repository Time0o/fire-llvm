#include <fire-llvm/fire.hpp>

#include <iostream>

namespace {

std::string fire_main_hello(std::string const &msg)
{
  return msg;
}

}

int main()
{
  fire::fire_llvm(fire_main_hello);
}
