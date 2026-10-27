# Observer mod: the Geode half of the trace harness.
#
# Build path (notes/trace-observer.md): the SDK is consumed as a subdirectory because its
# own CMakeLists is `project(geode-sdk ...)`, which is the target setup_geode_mod links.
# It needs `geode sdk install-binaries` to have created bin/<version>/.
#
#   geode build
#
# DONT_INSTALL keeps every build out of the game profiles until the user installs it.

cmake_minimum_required(VERSION 3.21)
project(megahack_trace_observer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MH_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_library(observer SHARED
    src/main.cpp
    ${MH_CORE_DIR}/src/contract.cpp
    ${MH_CORE_DIR}/src/value.cpp
    ${MH_CORE_DIR}/src/json.cpp
    ${MH_CORE_DIR}/src/json_parse.cpp
    ${MH_CORE_DIR}/src/sha256.cpp
    ${MH_CORE_DIR}/src/fingerprint.cpp
    ${MH_CORE_DIR}/src/trace_writer.cpp
    ${MH_CORE_DIR}/src/context.cpp
)
target_include_directories(observer PRIVATE "${MH_CORE_DIR}/include")

add_subdirectory("$ENV{GEODE_SDK}" "${CMAKE_BINARY_DIR}/geode-sdk")
include("$ENV{GEODE_SDK}/cmake/GeodeFile.cmake")

setup_geode_mod(observer DONT_INSTALL)
