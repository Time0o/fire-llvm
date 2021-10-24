cmake_minimum_required(VERSION 3.9)

set(run_test "${CMAKE_CURRENT_SOURCE_DIR}/run_test")

file(GLOB test_sources "test_*.cpp")

foreach(test_source ${test_sources})
  get_filename_component(test_prog ${test_source} NAME)
  string(REGEX REPLACE "test_(.*).cpp" "\\1" test_prog ${test_prog})

  add_executable(${test_prog} ${test_source})
  fire_llvm_config(${test_prog})

  add_test(NAME ${test_prog}
           COMMAND ${run_test} $<TARGET_FILE:${test_prog}>
           WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endforeach()
