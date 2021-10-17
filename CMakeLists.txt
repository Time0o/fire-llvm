cmake_minimum_required(VERSION 3.9)

project("LLVMFire"
        VERSION 0.1
        LANGUAGES CXX)

set(PROJECT_AUTHOR "Timo Nicolai")
set(PROJECT_COPYRIGHT "2021")

find_package(LLVM REQUIRED CONFIG)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

add_subdirectory(source)
