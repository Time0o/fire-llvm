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
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/MemoryBuffer.h"

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
  FireMatchCallback(clang::Rewriter *FileRewriter)
  : FileRewriter_(FileRewriter)
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

    FileRewriter_->ReplaceText(FireCall->getSourceRange(),
                               "std::printf(\"hello fire\")"); // XXX
  }

private:
  clang::Rewriter *FileRewriter_;
};

class FireConsumer : public clang::ASTConsumer
{
public:
  FireConsumer(clang::Rewriter *FileRewriter)
  : FileRewriter_(FileRewriter)
  {}

  void HandleTranslationUnit(clang::ASTContext &Context) override
  {
    using namespace clang::ast_matchers;

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
    FireMatchCallback MatchCallback(FileRewriter_);

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
  }

private:
  clang::Rewriter *FileRewriter_;
};

class FireAction : public clang::PluginASTAction
{
protected:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &CI,
    llvm::StringRef FileName) override
  {
    auto &SourceManager { CI.getSourceManager() };
    auto &LangOpts { CI.getLangOpts() };

    CI_ = &CI;

    FileName_ = FileName;
    FileID_ = SourceManager.getMainFileID(); // XXX
    FileRewriter_.setSourceMgr(SourceManager, LangOpts);

    return std::make_unique<FireConsumer>(&FileRewriter_);
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

  void EndSourceFileAction() override
  {
    auto &CodeGenOpts { CI_->getCodeGenOpts() };
    auto &Target { CI_->getTarget() };
    auto &Diagnostics { CI_->getDiagnostics() };

    // retrieve rewrite buffer
    auto FileRewriteBuffer { FileRewriter_.getRewriteBufferFor(FileID_) };

    std::string FileContent {
        FileRewriteBuffer->begin(), FileRewriteBuffer->end() };

    auto FileMemoryBuffer { llvm::MemoryBuffer::getMemBufferCopy(FileContent) };

    // create new compiler instance
    auto CInvNew { std::make_shared<clang::CompilerInvocation>() };

    assert(clang::CompilerInvocation::CreateFromArgs(
      *CInvNew, CodeGenOpts.CommandLineArgs, Diagnostics));

    clang::CompilerInstance CINew;
    CINew.setInvocation(CInvNew);
    CINew.setTarget(&Target);
    CINew.createDiagnostics();

    // create "virtual" input file
    auto &PreprocessorOpts { CINew.getPreprocessorOpts() };

    PreprocessorOpts.addRemappedFile(FileName_, FileMemoryBuffer.get());

    // generate code
    clang::EmitObjAction EmitObj;
    CINew.ExecuteAction(EmitObj);

    // clean up rewrite buffer
    FileMemoryBuffer.release();
  }

private:
  clang::CompilerInstance *CI_;

  std::string FileName_;
  clang::FileID FileID_;
  clang::Rewriter FileRewriter_;
};

} // end namespace

static clang::FrontendPluginRegistry::Add<FireAction>
X("fire", "create CLI from functions or classes");
