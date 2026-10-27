# Reconstruction target mod: the second variant of the runtime-validation comparison.
#
# Build recipe is the observer's, verified in notes/trace-observer.md §Environment: the SDK is
# consumed as a subdirectory because its own CMakeLists is `project(geode-sdk ...)`, which is
# the target `setup_geode_mod` links. It needs `geode sdk install-binaries` to have created
# `bin/<version>/`.
#
#   geode build
#
# What this mod deliberately does NOT link is the `mh_trace` core. There is exactly one
# emitter in a run - `megahack.trace` - because the comparability gate requires the same
# harness identity on both sides (harness/src/comparator.cpp, the `harness` row). A second
# writer would produce a second trace whose harness hash cannot match, which is a gate
# failure by construction rather than a comparison.

cmake_minimum_required(VERSION 3.21)
project(megahack_reconstruction_target CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(reconstruction SHARED src/main.cpp)

# AGENTS.md asks for an idempotent builder: "running it twice gives the same bytes".
# `/Brepro` removes the compiler's and linker's clock input, but it is not sufficient here -
# two relinks of this target still produce different digests (measured, notes/reconstruction-
# variant.md), because the SDK's toolchain also stamps debug information. So the manifest's
# `targets.reconstruction.sha256` is a per-build pin: re-fill it after any rebuild.
if(MSVC)
    target_compile_options(reconstruction PRIVATE /Brepro)
    target_link_options(reconstruction PRIVATE /Brepro)
endif()

add_subdirectory("$ENV{GEODE_SDK}" "${CMAKE_BINARY_DIR}/geode-sdk")
include("$ENV{GEODE_SDK}/cmake/GeodeFile.cmake")

setup_geode_mod(reconstruction DONT_INSTALL)
