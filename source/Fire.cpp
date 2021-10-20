#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"

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
  FireMatchCallback(clang::Rewriter *Rewriter)
  : Rewriter_(Rewriter)
  {}

  void run(clang::ast_matchers::MatchFinder::MatchResult const &Result) override
  {
    auto FireCall { Result.Nodes.getNodeAs<clang::CallExpr>("fire") };

    auto FireCallLoc = FireCall->getBeginLoc();

    if (FireCall->getNumArgs() != 1)
      throw FireError("fire::Fire expects exactly one argument", FireCallLoc);

    auto FireArg { llvm::dyn_cast<clang::DeclRefExpr>(FireCall->getArg(0)) };
    if (!FireArg)
      throw FireError("fire::Fire expects a function or class type argument", FireCallLoc);

    auto FireFunctionDecl { llvm::dyn_cast<clang::FunctionDecl>(FireArg->getDecl()) };
    assert(FireFunctionDecl); // XXX

    Rewriter_->ReplaceText(FireCall->getSourceRange(),
                           "std::printf(\"hello fire\");"); // XXX
  }

private:
  clang::Rewriter *Rewriter_;
};

class FireConsumer : public clang::ASTConsumer
{
public:
  FireConsumer(std::string *FileRewritten)
  : FileRewritten_(FileRewritten)
  {}

  void HandleTranslationUnit(clang::ASTContext &Context) override
  {
    using namespace clang::ast_matchers;

    // create file rewriter
    auto &SourceManager { Context.getSourceManager() };
    auto &LangOpts { Context.getLangOpts() };

    clang::Rewriter Rewriter;
    Rewriter.setSourceMgr(SourceManager, LangOpts);

    // create match expression
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

    // create match callback
    FireMatchCallback MatchCallback(&Rewriter);

    // create and run match finder
    clang::ast_matchers::MatchFinder MatchFinder;
    MatchFinder.addMatcher(FireMatchExpression.bind("fire"), &MatchCallback);

    try {
      MatchFinder.matchAST(Context);

    } catch (FireError const &e) {
      auto &Diags { Context.getDiagnostics() };

      unsigned ID { Diags.getDiagnosticIDs()->getCustomDiagID(
                      clang::DiagnosticIDs::Error, e.what()) };

      Diags.Report(e.where(), ID);
    }

    // copy rewritten file to buffer
    auto FileID { SourceManager.getMainFileID() };
    auto FileRewriteBuffer { Rewriter.getRewriteBufferFor(FileID) };

    *FileRewritten_ = std::string(FileRewriteBuffer->begin(),
                                  FileRewriteBuffer->end());
  }

private:
  std::string *FileRewritten_;
};

class FireAction : public clang::PluginASTAction
{
protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &CI,
    llvm::StringRef) override
  {
    return std::make_unique<FireConsumer>(&FileRewritten_);
  }

  bool ParseArgs(clang::CompilerInstance const &,
                 std::vector<std::string> const &) override
  {
    return true;
  }

  clang::PluginASTAction::ActionType getActionType() override
  {
    return clang::PluginASTAction::ReplaceAction;
  }

  bool BeginSourceFileAction (clang::CompilerInstance &)
  {
    FileRewritten_.clear();

    return true;
  }

  void EndSourceFileAction() override
  {
    // XXX
  }

private:
  std::string FileRewritten_;
};

} // end namespace

static clang::FrontendPluginRegistry::Add<FireAction>
X("fire", "create CLI from functions or classes");
