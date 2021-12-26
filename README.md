# fire-llvm

fire-llvm is a Clang plugin that can be used to turn ordinary C++ functions and
objects into command line interfaces (CLIs) similarly to
[python-fire](https://github.com/google/python-fire).

## Basic example

Given the following source file `calc.cc`:

```c++
#include <fire-llvm/fire.hpp>

struct Calculator
{
  int add(int a, int b)
  {
    return a + b;
  }

  int sub(int a, int b)
  {
    return a - b;
  }
};

Calculator calc;

int main()
{
  fire::fire_llvm(calc);
}
```

run `clang++ -Xclang -load -Xclang $FIRE_LLVM_PLUGIN -Xclang -add-plugin
-Xclang fire -I fire-llvm/include calc.cc -o calc` to create an executable
`calc`. Here, `$FIRE_LLVM_PLUGIN` should expand to the location of the shared
object created when building this plugin [Installation](#installation).
Alternatively, if you're building `calc` with CMake you could use the
`fire_llvm_config` function:

```cmake
add_executable(calc calc.cc)
fire_llvm_config(calc)
```

You can then use `calc` as follows:

```
$> ./calc add -a=1 -b=2
3
$> ./calc sub -a=1 -b=2
-1
```

For more examples, take a look at the tests in the `tests` directory.

## Installation

To build fire-llvm you will need the LLVM development libraries. I have tested
this with Clang/LLVM version 11 and 12 minor tweaks might be needed for other
versions.

To build the plugin, run:

```
mkdir build
cd build
cmake ..
make
```

which will create the plugin under
`build/fire-llvm/plugin/libfire-llvm-plugin.so`.

## Ackknowledgements

fire-llvm is based on [fire-hpp](https://github.com/kongaskristjan/fire-hpp).
