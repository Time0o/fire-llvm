#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"

namespace {

class FireError : public std::exception
{
public:
  FireError(std::string const &What, clang::SourceLocation const &Where)
  : What_(What),
    Where_(Where)
  {}

  char const *what() const noexcept override
  { return What_.c_str(); }

  clang::SourceLocation where() const noexcept
  { return Where_; }

private:
  std::string What_;
  clang::SourceLocation Where_;
};

class FireMatchCallback : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
  void run(clang::ast_matchers::MatchFinder::MatchResult const &Result) override
  {
    auto FireCall { Result.Nodes.getNodeAs<clang::CallExpr>("fire") };

    auto FireCallLoc = FireCall->getCalleeDecl()->getLocation();

    if (FireCall->getNumArgs() != 1)
      throw FireError("fire::Fire expects exactly one argument", FireCallLoc);

    auto FireArg { llvm::dyn_cast<clang::DeclRefExpr>(FireCall->getArg(0)) };
    if (!FireArg)
      throw FireError("fire::Fire expects a function or class type argument", FireCallLoc);

    auto FireFunctionDecl { llvm::dyn_cast<clang::FunctionDecl>(FireArg->getDecl()) };
    assert(FireFunctionDecl); // XXX
  }
};

class FireConsumer : public clang::ASTConsumer
{
public:
  FireConsumer()
  {
    using namespace clang::ast_matchers;

    auto FireMatchExpression {
      callExpr(
        callee(
          functionDecl(
            allOf(
              hasName("Fire"),
              hasAncestor( // XXX
                namespaceDecl(
                  hasName("fire")))))))
    };

    FireMatchCallback MatchCallback;

    MatchFinder_.addMatcher(FireMatchExpression.bind("fire"), &MatchCallback);
  }

  void HandleTranslationUnit(clang::ASTContext &Context) override
  {
    try {
      MatchFinder_.matchAST(Context);

    } catch (FireError const &e) {
      auto &Diags { Context.getDiagnostics() };

      unsigned ID { Diags.getDiagnosticIDs()->getCustomDiagID(
                      clang::DiagnosticIDs::Error, e.what()) };

      Diags.Report(e.where(), ID);
    }
  }

private:
  clang::ast_matchers::MatchFinder MatchFinder_;
};

class FireAction : public clang::PluginASTAction
{
protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &,
    llvm::StringRef) override
  {
    return std::make_unique<FireConsumer>();
  }

  bool ParseArgs(clang::CompilerInstance const &,
                 std::vector<std::string> const &) override
  {
    return true; // XXX
  }

  // XXX ActionType?
};

} // end namespace

static clang::FrontendPluginRegistry::Add<FireAction>
X("fire", "create CLI from functions or classes");
