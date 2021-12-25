cmake_minimum_required(VERSION 3.9)

add_library(fire-llvm-plugin MODULE fire.cpp)
target_compile_features(fire-llvm-plugin PRIVATE cxx_std_17)

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS})
