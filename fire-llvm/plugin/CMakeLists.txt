cmake_minimum_required(VERSION 3.9)

add_library(fire-llvm-plugin MODULE fire.cpp)
target_compile_features(fire-llvm-plugin PRIVATE cxx_std_17)
clang_config(fire-llvm-plugin)
