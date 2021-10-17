#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"

#include "llvm/ADT/StringRef.h"

namespace {

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
              hasParent(
                namespaceDecl(
                  hasName("fire")))))))
    };

    class FireMatchCallback : public MatchFinder::MatchCallback
    {
    public:
      void run(MatchFinder::MatchResult const &Result) override
      {
        auto Node { Result.Nodes.getNodeAs<clang::CallExpr>("fire") };
        assert(Node);

        // XXX
      }
    };

    MatchFinder MatchFinder;
    FireMatchCallback MatchCallback;

    MatchFinder.addMatcher(FireMatchExpression.bind("fire"), &MatchCallback);
  }
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
};

} // end namespace

static clang::FrontendPluginRegistry::Add<FireAction>
X("fire", "create CLI from functions or classes");
