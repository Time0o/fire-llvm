#pragma once

#include <string>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

#include "namespace.hpp"

namespace type {

inline bool is(clang::QualType Type,
               std::string const &Name,
               std::string const &Namespace = "")
{
  auto R { Type->getAs<clang::RecordType>() };
  if (!R)
    return false;

  auto RD { R->getDecl() };
  if (!RD)
    return false;

  if (!Namespace.empty() && !ns::isIn(RD, Namespace))
    return false;

  return RD->getName() == Name;
}

inline bool isTemplate(clang::QualType Type,
                       std::string const &Name,
                       std::string const &Namespace = "")
{
  auto TS { Type->getAs<clang::TemplateSpecializationType>() };
  if (!TS)
    return false;

  auto TM { TS->getTemplateName() };
  auto TD { TM.getAsTemplateDecl() };
  if (!TD)
    return false;

  if (!Namespace.empty() && !ns::isIn(TD, Namespace))
    return false;

  return TD->getName() == Name;
}

} // end namespace type
