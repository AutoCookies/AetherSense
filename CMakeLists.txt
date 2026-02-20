cmake_minimum_required(VERSION 3.20)
project(AetherSense VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AETHERSENSE_ENABLE_SANITIZERS "Enable ASAN/UBSAN" OFF)

if(AETHERSENSE_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(aethersense_core
  src/core/config.cpp
  src/io/csv_reader.cpp
  src/io/json_reader.cpp
  src/runtime/ring_buffer.cpp
  src/runtime/pipeline.cpp
)

target_include_directories(aethersense_core PUBLIC include)
target_compile_options(aethersense_core PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_executable(aethersense_cli apps/aethersense_cli.cpp)
set_target_properties(aethersense_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/apps)
target_link_libraries(aethersense_cli PRIVATE aethersense_core)

include(CTest)
if(BUILD_TESTING)
  add_executable(aethersense_tests
    tests/test_main.cpp
    tests/test_config.cpp
    tests/test_ring_buffer.cpp
    tests/test_parsers.cpp
    tests/test_dsp.cpp
    tests/test_decision.cpp
    tests/test_pipeline_smoke.cpp
  )
  target_link_libraries(aethersense_tests PRIVATE aethersense_core)
  add_test(NAME aethersense_tests COMMAND aethersense_tests)
endif()
