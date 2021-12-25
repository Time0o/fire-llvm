#include <cassert>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Type.h"
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
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/MemoryBuffer.h"

#include "compile.hpp"
#include "node.hpp"
#include "print.hpp"
#include "string.hpp"
#include "type.hpp"

namespace {

class FireError : public std::exception
{
public:
  FireError(std::string const &What, clang::SourceLocation const &Where)
  : What_(What),
    Where_(Where)
  {}

  template<typename T>
  FireError(std::string const &What, T const *Where)
  : FireError(What, Where->getBeginLoc())
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
  FireMatchCallback(clang::ASTContext &Context,
                    clang::FileID *FileID,
                    clang::Rewriter *FileRewriter)
  : Context_(Context),
    FileID_(FileID),
    FileRewriter_(FileRewriter)
  {}

  void run(clang::ast_matchers::MatchFinder::MatchResult const &Result) override
  {
    // Locate fire::fire_llm call

    auto FireCall { Result.Nodes.getNodeAs<clang::CallExpr>("fire") };

    if (FireCall->getNumArgs() != 1)
      throw FireError("fire::fire_llvm expects exactly one argument", FireCall);

    auto FireCallArg { llvm::dyn_cast<clang::DeclRefExpr>(FireCall->getArg(0)) };
    if (!FireCallArg)
      throw FireError("fire::fire_llvm expects a function or class type argument", FireCall);

    // Locate main function.

    clang::FunctionDecl const *Main { nullptr };

    for (auto const &FireCallAncestor : node::getAncestors(Context_, *FireCall)) {
      auto MaybeMain { FireCallAncestor.get<clang::FunctionDecl>() };

      if (MaybeMain && MaybeMain->isMain()) {
        Main = MaybeMain;
        break;
      }
    }

    if (!Main)
      throw FireError("fire::fire_llvm must be called inside 'main'", FireCall);

    // Replace main function.

    std::string FireMain;

    auto FireCallArgDecl { FireCallArg->getDecl() };

    if (llvm::isa<clang::FunctionDecl>(FireCallArgDecl)) {
      auto Function { llvm::dyn_cast<clang::FunctionDecl>(FireCallArgDecl) };

      FireMain = fireMainFunction(Function);

    } else if (llvm::isa<clang::CXXRecordDecl>(FireCallArgDecl)) {
      auto Record { llvm::dyn_cast<clang::FunctionDecl>(FireCallArgDecl) };

      // XXX

    } else {
      throw FireError("fire::fire_llvm expects a function or class type argument", FireCall);
    }

    FileRewriter_->ReplaceText(Main->getSourceRange(), FireMain);

    // Obtain file ID of main function.

    auto &SourceManager { Context_.getSourceManager() };

    *FileID_ = SourceManager.getFileID(Main->getBeginLoc());
  }

private:
  std::string fireMainFunction(clang::FunctionDecl const *Function) const
  {
    auto FunctionName { Function->getName() };

    for (auto Param : Function->parameters())
      FileRewriter_->ReplaceText(Param->getSourceRange(), fireParam(Param));

    return llvm::formatv("FIRE({0})", FunctionName);
  }

  std::string fireParam(clang::ParmVarDecl const *Param) const
  {
    // Obtain parameter name/type/default value.

    auto ParamName { Param->getName() };
    if (ParamName.empty())
        throw FireError("Parameter must not be unnamed", Param);

    auto ParamType { Param->getType() };
    if (ParamType->isLValueReferenceType() &&
        ParamType.getNonReferenceType().isConstQualified())
      ParamType = ParamType.getNonReferenceType();

    std::string ParamDefault;
    if (Param->hasDefaultArg())
      ParamDefault = print::source(Context_, Param->getDefaultArgRange());

    // Construct fire parameter name/type/default value.

    std::string FireParamName { ParamName };

    std::string FireParamType { print::type(Context_, Param->getType()) };

    std::string FireParamDefault;

    if (type::isTemplate(ParamType, "vector", "std")) {
      FireParamDefault = "fire::arg(fire::variadic())";

    } else {
      if (type::isTemplate(ParamType, "optional", "std")) {
        str::replace(FireParamType, "std::optional", "fire::optional");

      } else if (!type::is(ParamType, "basic_string") &&
                 !ParamType->isBooleanType() &&
                 !ParamType->isIntegerType() &&
                 !ParamType->isFloatingType()) {

        throw FireError(
          "Parameter must have boolean, integral or floating point type or be"
          "one of std::string, std::vector, std::optional", Param);
      }

      auto FireParamDash { FireParamName.size() > 1 ? "--" : "-" };

      if (ParamDefault.empty()) {
        FireParamDefault = llvm::formatv("fire::arg(\"{0}{1}\")",
                                         FireParamDash,
                                         FireParamName);
      } else {
        FireParamDefault = llvm::formatv("fire::arg(\"{0}{1}\", {2})",
                                         FireParamDash,
                                         FireParamName,
                                         ParamDefault);
      }
    }

    return llvm::formatv("{0} {1} = {2}",
                         FireParamType,
                         FireParamName,
                         FireParamDefault);
  }

  clang::ASTContext &Context_;
  clang::FileID *FileID_;
  clang::Rewriter *FileRewriter_;
};

class FireConsumer : public clang::ASTConsumer
{
public:
  FireConsumer(clang::FileID *FileID,
               clang::Rewriter *FileRewriter,
               bool *FileRewriteError)
  : FileID_(FileID),
    FileRewriter_(FileRewriter),
    FileRewriteError_(FileRewriteError)
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
              hasName("fire_llvm"),
              hasParent(
                functionTemplateDecl(
                  hasParent(
                    namespaceDecl(
                      hasName("fire")))))))))
    };

    // create match callback
    FireMatchCallback MatchCallback(Context, FileID_, FileRewriter_);

    // create and run match finder
    clang::ast_matchers::MatchFinder MatchFinder;
    MatchFinder.addMatcher(FireMatchExpression.bind("fire"), &MatchCallback);

    try {
      MatchFinder.matchAST(Context);

      *FileRewriteError_ = false;

    } catch (FireError const &e) {
      auto &Diags { Context.getDiagnostics() };

      unsigned ID { Diags.getDiagnosticIDs()->getCustomDiagID(
                      clang::DiagnosticIDs::Error, e.what()) };

      Diags.Report(e.where(), ID);

      *FileRewriteError_ = true;
    }
  }

private:
  clang::FileID *FileID_;
  clang::Rewriter *FileRewriter_;
  bool *FileRewriteError_;
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

    FileName_ = FileName.str();

    FileRewriter_.setSourceMgr(SourceManager, LangOpts);

    return std::make_unique<FireConsumer>(
        &FileID_, &FileRewriter_, &FileRewriteError_);
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
    if (FileRewriteError_)
      return;

    auto FileRewriteBuffer { FileRewriter_.getRewriteBufferFor(FileID_) };

    compile(CI_, FileName_, FileRewriteBuffer->begin(), FileRewriteBuffer->end());
  }

private:
  clang::CompilerInstance *CI_;

  std::string FileName_;
  clang::FileID FileID_;
  clang::Rewriter FileRewriter_;
  bool FileRewriteError_ = false;
};

} // end namespace

static clang::FrontendPluginRegistry::Add<FireAction>
X("fire", "create CLI from functions or classes");
