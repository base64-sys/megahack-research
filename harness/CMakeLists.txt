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
)

target_include_directories(mh_trace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(mh_trace PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(mh_trace PRIVATE /W4 /permissive-)
else()
  target_compile_options(mh_trace PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_subdirectory(tests)
