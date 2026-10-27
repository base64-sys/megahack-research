# Host-testable trace core for runtime validation.
#
# Contract: notes/runtime-validation.md. Everything here is plain C++20: no Geode
# headers, no Windows API, no third-party dependencies. The Geode observer that emits
# traces lives in the mod tree, not here; this tree only decides what a trace means.

cmake_minimum_required(VERSION 3.20)
project(mh_trace LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_library(mh_trace STATIC
  src/contract.cpp
  src/value.cpp
  src/json.cpp
  src/json_parse.cpp
  src/sha256.cpp
  src/fingerprint.cpp
  src/trace_writer.cpp
  src/call_stack.cpp
  src/trace_reader.cpp
  src/scenario.cpp
  src/comparator.cpp
  src/sequence.cpp
  src/comparison.cpp
  src/compare.cpp
  src/context.cpp
)

target_include_directories(mh_trace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(mh_trace PUBLIC cxx_std_20)

# Host tool: grade a captured trace without writing a test (harness/OBSERVER-PROGRESS.md O4c).
add_executable(mh_trace_check tools/trace_check.cpp)
target_link_libraries(mh_trace_check PRIVATE mh_trace)
target_compile_features(mh_trace_check PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(mh_trace_check PRIVATE /W4 /permissive-)
else()
  target_compile_options(mh_trace_check PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(MSVC)
  target_compile_options(mh_trace PRIVATE /W4 /permissive-)
else()
  target_compile_options(mh_trace PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_subdirectory(tests)

# The validator CLI over the note's own example: valid, exit 0 (harness/OBSERVER-PROGRESS.md O4c).
add_test(NAME trace_check_example
         COMMAND mh_trace_check ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/example_trace.jsonl)
add_test(NAME trace_check_usage COMMAND mh_trace_check)
set_tests_properties(trace_check_usage PROPERTIES WILL_FAIL TRUE)


add_executable(mh_scenario_check tools/scenario_check.cpp)
target_link_libraries(mh_scenario_check PRIVATE mh_trace)
target_compile_features(mh_scenario_check PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(mh_scenario_check PRIVATE /W4 /permissive-)
else()
  target_compile_options(mh_scenario_check PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME scenario_check_example
         COMMAND mh_scenario_check ${CMAKE_CURRENT_SOURCE_DIR}/scenarios/safe-mode-smoke.json)
add_test(NAME scenario_check_usage COMMAND mh_scenario_check)
set_tests_properties(scenario_check_usage PROPERTIES WILL_FAIL TRUE)
