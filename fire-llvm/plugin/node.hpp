#pragma once

#include <cstdint>
#include <queue>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"

namespace node {

template<typename T>
std::vector<clang::DynTypedNode> getAncestors(clang::ASTContext &Context,
                                              T const &Node)
{
  std::vector<clang::DynTypedNode> Ancestors;

  std::queue<clang::DynTypedNode> ParentQueue;

  for (auto const &Parent : Context.getParents(Node))
    ParentQueue.push(Parent);

  while (!ParentQueue.empty()) {
    std::size_t NumParents { ParentQueue.size() };

    for (std::size_t i = 0; i < NumParents; ++i) {
      auto Parent { ParentQueue.front() };
      ParentQueue.pop();

      Ancestors.push_back(Parent);

      for (auto const &GrandParent : Context.getParents(Parent))
        ParentQueue.push(GrandParent);
    }
  }

  return Ancestors;
}

} // end namespace node
