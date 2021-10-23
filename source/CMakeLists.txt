add_library(LLVMFire MODULE LLVMFire.cpp)

add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS})
