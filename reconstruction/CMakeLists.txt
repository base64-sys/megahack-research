cmake_minimum_required(VERSION 3.20)
project(megahack_reconstruction CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(hackpro_core STATIC
    src/config.cpp
    src/extensions.cpp
    src/entry.cpp
)

add_executable(test_reconstruction
    tests/test_reconstruction.cpp
)

target_link_libraries(test_reconstruction PRIVATE hackpro_core)
