#include <fire-llvm/fire.hpp>

namespace {

int default_arg(int d = 0)
{
  return d;
}

}

int main()
{
  fire::fire_llvm(default_arg);
}
