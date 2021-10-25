cmake_minimum_required(VERSION 3.9)

project("fire-llvm"
        VERSION 0.1
        LANGUAGES CXX)

include(CTest)

option(FIRE_LLVM_ENABLE_TESTING "Enable tests" ON)

find_package(LLVM REQUIRED CONFIG)

set(FIRE_EXAMPLES OFF CACHE BOOL "Compile fire-hpp examples")
set(FIRE_UNIT_TESTS OFF CACHE BOOL "Enable fire-hpp unit tests")
add_subdirectory(fire-hpp EXCLUDE_FROM_ALL)

add_subdirectory(fire-llvm)

function(fire_llvm_config TARGET)
  set(options DISABLED)
  set(oneValueArgs)
  set(multiValueArgs)
  cmake_parse_arguments(FIRE_LLVM_CONFIG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "fire_llvm_config can only be used when CMAKE_CXX_COMPILER is Clang")
  endif()

  get_target_property(target_type ${TARGET} TYPE)
  if (NOT target_type STREQUAL "EXECUTABLE")
    message(FATAL_ERROR "fire_llvm_config can only be used on executable targets")
  endif()

  if (NOT ${FIRE_LLVM_CONFIG_DISABLED})
    target_compile_options(${TARGET} PRIVATE
      "SHELL:-Xclang -load"
      "SHELL:-Xclang $<TARGET_FILE:fire-llvm-plugin>"
      "SHELL:-Xclang -add-plugin"
      "SHELL:-Xclang fire")
  endif()

  target_link_libraries(${TARGET} PRIVATE fire-hpp fire-llvm)

  # XXX rebuild when fire-llvm-plugin changes
endfunction()

if (FIRE_LLVM_ENABLE_TESTING)
  add_subdirectory(tests)
endif()
