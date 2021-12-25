#pragma once

#include <string>

#include "clang/AST/Decl.h"

#include "llvm/Support/Casting.h"

namespace ns
{

inline bool isIn(clang::Decl const *Decl, std::string const &Namespace)
{
   auto ND { llvm::dyn_cast<clang::NamespaceDecl>(Decl->getDeclContext()) };
   if (!ND)
     return false;

   auto II { ND->getIdentifier() };
   if (!II || II->getName() != Namespace)
     return false;

   return llvm::isa<clang::TranslationUnitDecl>(ND->getDeclContext());
}

} // end namespace ns
