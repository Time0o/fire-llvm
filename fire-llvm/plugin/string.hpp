#pragma once

#include <string>

namespace str {

static void replace(std::string &Str,
                    std::string const &From,
                    std::string const &To)
{
  auto It { Str.find(From) };
  if (It != std::string::npos)
    Str.replace(It, From.size(), To);
}

} // end namespace str
