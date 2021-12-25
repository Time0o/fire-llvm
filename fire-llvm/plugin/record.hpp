#pragma once

#include <string>
#include <utility>
#include <vector>

#include "clang/AST/DeclCXX.h"

#include "llvm/Support/Casting.h"

namespace record {

std::pair<std::vector<clang::CXXMethodDecl const *>, std::string>
publicMethods(clang::CXXRecordDecl const *Record)
{
  std::vector<clang::CXXMethodDecl const *> publicMethods;
  std::string publicMethodOptions;

  for (auto Method : Record->methods()) {
    // Skip non-public methods.
    if (Method->getAccess() != clang::AS_public)
      continue;

    // Skip constructors and operators.
    if (llvm::isa<clang::CXXConstructorDecl>(Method) ||
        llvm::isa<clang::CXXDestructorDecl>(Method) ||
        Method->isCopyAssignmentOperator() ||
        Method->isMoveAssignmentOperator() ||
        Method->isOverloadedOperator())
      continue;

    publicMethods.push_back(Method);

    auto MethodName { Method->getNameAsString() };

    if (publicMethodOptions.empty())
      publicMethodOptions = MethodName;
    else
      publicMethodOptions += "|" + MethodName;
  }

  return { publicMethods, publicMethodOptions };
}

} // end namespace record
