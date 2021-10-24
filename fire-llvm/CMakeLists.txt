cmake_minimum_required(VERSION 3.9)

add_library(fire-llvm INTERFACE)
target_include_directories(fire-llvm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(plugin)
