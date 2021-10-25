#include <fire-llvm/fire.hpp>

#include <iostream>
#include <vector>

namespace {

int fire_main_variadic(std::vector<int> const &variadic)
{
  std::cout << "variadic = {";

  if (!variadic.empty()) {
    std::cout << variadic[0];
    for (std::size_t i = 1; i < variadic.size(); ++i)
      std::cout << ", " << variadic[i];
  }

  std::cout << "}";

  return 0;
}

}

int main()
{
  fire::fire_llvm(fire_main_variadic);
}
