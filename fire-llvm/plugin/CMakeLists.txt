cmake_minimum_required(VERSION 3.9)

add_library(fire-llvm-plugin MODULE fire.cpp)
target_compile_features(fire-llvm-plugin PRIVATE cxx_std_17)
target_link_libraries(fire-llvm-plugin PRIVATE clang-cpp)
llvm_config(fire-llvm-plugin USE_SHARED option)
target_include_directories(fire-llvm-plugin PRIVATE ${LLVM_INCLUDE_DIRS})
