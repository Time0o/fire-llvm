#include <fire-llvm/fire.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct S
{
  std::string hello(std::string const &msg)
  {
    return msg;
  }

  int add(int a, int b)
  {
    return a + b;
  }

  bool flag(bool f)
  {
    return f;
  }

  int default_arg(int d = 0)
  {
    return d;
  }

  void optional(std::optional<int> opt)
  {
    if (opt)
      std::cout << "opt = " << opt.value() << std::endl;
    else
      std::cout << "opt = nothing" << std::endl;
  }

  void variadic(std::vector<int> const &variadic)
  {
    std::cout << "variadic = {";

    if (!variadic.empty()) {
      std::cout << variadic[0];
      for (std::size_t i = 1; i < variadic.size(); ++i)
        std::cout << ", " << variadic[i];
    }

    std::cout << "}";
  }
};

S s;

int main()
{
  fire::fire_llvm(s);
}
