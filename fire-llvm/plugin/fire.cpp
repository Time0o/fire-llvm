#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
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
#include "clang/Rewrite/Core/Rewriter.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FormatVariadic.h"

#include "compile.hpp"
#include "node.hpp"
#include "print.hpp"
#include "record.hpp"
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

    } else if (llvm::isa<clang::ValueDecl>(FireCallArgDecl)) {
      auto Value { llvm::dyn_cast<clang::ValueDecl>(FireCallArgDecl) };

      auto Record { Value->getType()->getAsCXXRecordDecl() };
      auto RecordInstance { Value->getNameAsString() };

      if (Record)
        FireMain = fireMainRecord(Record, RecordInstance);
    }

    if (FireMain.empty())
      throw FireError("fire::fire_llvm expects a function or class type argument", FireCall);

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

  std::string fireMainRecord(clang::CXXRecordDecl const *Record,
                             std::string const &RecordInstance) const
  {
    // Public methods.
    auto [publicMethods, publicMethodOptions] = record::publicMethods(Record);

    // Code generation helper functions.

    auto RecordName { Record->getNameAsString() };

    // Names of forwarding launchpad functions.
    auto Launch = [&RecordName](std::string const &MethodName, bool Fire = false)
    { return RecordName + "_" + MethodName + (Fire ? "_fire" : ""); };

    // Name of new main entry point.
    auto LaunchEntry = [&RecordName]()
    { return RecordName + "_main"; };

    // Code generation.

    std::stringstream SS;

    // Begin detail namespace.
    SS << "namespace fire::detail {\n\n";

    // Launchpad functions.
    for (auto Method : publicMethods) {
      auto MethodName { Method->getNameAsString() };
      auto MethodReturnType { print::type(Context_, Method->getReturnType()) };
      auto MethodNumParams { Method->getNumParams() };

      // Launchpad function header.
      SS << MethodReturnType << " " << Launch(MethodName);

      SS << "(";
      for (unsigned i { 0 }; i < MethodNumParams; ++i) {
        auto MethodParam { Method->getParamDecl(i) };

        SS << fireParam(MethodParam);

        if (i + 1 < MethodNumParams)
          SS << ", ";
      }
      SS << ")\n";

      // Launchpad function body.
      SS << "{ ";

      if (MethodReturnType != "void")
        SS << "return ";

      SS << RecordInstance << "." << MethodName;

      SS << "(";
      for (unsigned i { 0 }; i < MethodNumParams; ++i) {
        auto MethodParam { Method->getParamDecl(i) };
        auto MethodParamName { MethodParam->getNameAsString() };

        SS << MethodParamName;

        if (i + 1 < MethodNumParams)
          SS << ", ";
      }
      SS << ")";

      SS << "; }\n\n";

      // FIRE_LAUNCH() call.
      std::string FireFunc {
        llvm::formatv("FIRE_LAUNCH({0}, {1})\n\n",
                      Launch(MethodName, true),
                      Launch(MethodName)) };

      SS << FireFunc;
    }

    // Entry point header.
    std::string LaunchEntryerHeader {
      llvm::formatv(
        "int {0}(std::string method = fire::arg({0, \"<method>\", \"({1})\"}))\n",
        LaunchEntry(),
        publicMethodOptions) };

    SS << LaunchEntryerHeader;

    // Entry point body.
    SS << "{\n";

    for (auto Method : publicMethods) {
      auto MethodName { Method->getNameAsString() };

      std::string LaunchEntryBranch {
        llvm::formatv(
          "if (method == \"{0}\") {1}({2}, {3});\n",
          MethodName,
          Launch(MethodName, true),
          "fire::raw_args.argc() - 1",
          "const_cast<const char **>(fire::raw_args.argv()) + 1") };

      SS << LaunchEntryBranch;
    }

    SS << "  return 0;\n";

    SS << "}\n\n";

    // End detail namespace.
    SS << "} // end namespace fire::detail\n\n";

    // FIRE() call.
    std::string Fire {
      llvm::formatv("FIRE_LAUNCH_ENTRY(fire::detail::{0});", LaunchEntry()) };

    SS << Fire;

    return SS.str();
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
