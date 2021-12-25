#pragma once

#include <memory>
#include <string>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"

#include "llvm/Support/MemoryBuffer.h"

template<typename IT>
inline void compile(clang::CompilerInstance *CI,
                    std::string const &FileName,
                    IT FileBegin,
                    IT FileEnd)
{
  auto &CodeGenOpts { CI->getCodeGenOpts() };
  auto &Target { CI->getTarget() };
  auto &Diagnostics { CI->getDiagnostics() };

  // create new compiler instance
  auto CInvNew { std::make_shared<clang::CompilerInvocation>() };

  assert(clang::CompilerInvocation::CreateFromArgs(
    *CInvNew, CodeGenOpts.CommandLineArgs, Diagnostics));

  clang::CompilerInstance CINew;
  CINew.setInvocation(CInvNew);
  CINew.setTarget(&Target);
  CINew.createDiagnostics();

  // create rewrite buffer
  std::string FileContent { FileBegin, FileEnd };
  auto FileMemoryBuffer { llvm::MemoryBuffer::getMemBufferCopy(FileContent) };

  // create "virtual" input file
  auto &PreprocessorOpts { CINew.getPreprocessorOpts() };
  PreprocessorOpts.addRemappedFile(FileName, FileMemoryBuffer.get());

  // generate code
  clang::EmitObjAction EmitObj;
  CINew.ExecuteAction(EmitObj);

  // clean up rewrite buffer
  FileMemoryBuffer.release();
}
