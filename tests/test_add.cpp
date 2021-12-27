#include <fire-llvm/fire.hpp>

namespace {

int add(int a, int b)
{
  return a + b;
}

}

int main()
{
  fire::fire_llvm(add);
}
