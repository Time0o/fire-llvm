#include <fire-llvm/fire.hpp>

namespace {

bool flag(bool f)
{
  return f;
}

}

int main()
{
  fire::fire_llvm(flag);
}
