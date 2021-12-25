#pragma once

#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceLocation.h"

namespace print {

inline std::string type(clang::ASTContext const &Context,
                        clang::QualType const &Type)
{
  auto &LangOpts { Context.getLangOpts() };

  clang::PrintingPolicy PP { LangOpts };

  return Type.getAsString(PP);
}

inline std::string source(clang::ASTContext const &Context,
                          clang::SourceRange const &Range)
{
  auto &SourceManager { Context.getSourceManager() };

  auto Begin { SourceManager.getCharacterData(Range.getBegin()) };
  auto End { SourceManager.getCharacterData(Range.getEnd()) };

  return std::string(Begin, End - Begin + 1);
}

} // end namespace print
