# Trace observer: build path and first findings

- **Question** — the trace core is done ([trace-harness.md](trace-harness.md)); what does it
  take to build the Geode observer that emits those traces on this machine, and does the
  installed SDK support it?
- **Method** — drive the installed `geode` CLI and its CMake files directly, in a throwaway
  project under the temp directory, and record every failure rather than guessing a template.
  The probe project is not part of the repository.
- **Date** — 2026-10-27.

## Environment (measured)

| Fact | Value | How |
| --- | --- | --- |
| CLI | `geode` 3.9.0 | `geode --help`, configure log |
| SDK env | `GEODE_SDK=C:\Users\Robert_\Documents\Geode` | process environment |
| SDK tree | loader source, `VERSION` = 5.11.2 | `CMakeLists.txt`, `loader/`, `installer/`, `bin/` |
| Prebuilt loader | `bin/Geode.dll`, `Geode.lib`, `5.11.1/`, `5.10.1/` | directory listing |
| Compiler | MSVC 14.44.35207 via VS 2022 BuildTools | `geode build` configure log |
| Windows SDK | 10.0.26100.0 | configure log |
| Profiles | `a` (Steam install, default), `runtime511` (isolated copy) | `geode profile list` |

## What was tried, and what failed

1. `geode new <dir>` — **fails without a TTY**: `Unable to get template: IO error: not a
   terminal`. The template selector cannot be scripted, so a project must be written by hand.
2. Project with `setup_geode_mod()` and no include — `Unknown CMake command
   "setup_geode_mod"`. The CLI does not inject the SDK's CMake.
3. `include($ENV{GEODE_SDK}/cmake/GeodeFile.cmake)` only — configure fails inside
   `GeodeFile.cmake:90`: `string sub-command JSON member 'gd ' not found`, because
   `GEODE_TARGET_PLATFORM_JSON` comes from `cmake/Platform.cmake`, which was never included.
4. Adding `Platform.cmake` with the SDK cmake directory on `CMAKE_MODULE_PATH` — fails at
   `Platform.cmake:1`: `include could not find requested file: cmake/PlatformDetect.cmake`.
   The include is source-relative, so these files only work inside the SDK tree itself.
5. Hand-rolled target: `add_library(... SHARED)` + `loader/include` + `bin/Geode.lib` —
   compiles, then fails on `Geode/Bindings.hpp(9): Cannot open include file:
   'Geode/Enums.hpp'`.
6. `geode sdk install C:\Users\Robert_\Documents\geode-sdk-511` — refused: `SDK is already
   installed at C:\Users\Robert_\Documents\Geode`; only `--reinstall` (which uninstalls) or
   `--force` change that, and neither is worth risking the existing checkout for.
7. **Working attempt** — consume the SDK as a subdirectory, which is what it is designed for:

   ```cmake
   cmake_minimum_required(VERSION 3.21)
   project(<mod> CXX)
   set(CMAKE_CXX_STANDARD 20)
   add_library(<mod> SHARED main.cpp)
   add_subdirectory("$ENV{GEODE_SDK}" "${CMAKE_BINARY_DIR}/geode-sdk")
   include("$ENV{GEODE_SDK}/cmake/GeodeFile.cmake")
   setup_geode_mod(<mod> DONT_INSTALL)
   ```

   The SDK's own `CMakeLists.txt` is `project(geode-sdk ...)` (line 97), so it defines the
   `geode-sdk` target that `setup_geode_mod` links. It fetches its dependencies with CPM,
   downloads a prebuilt `Codegen.exe` ("Downloading Codegen … Skipping building Codegen"),
   and runs it over the `bindings/2.2081` package to produce the generated headers.
   First run failed at `CMakeLists.txt:403`: `No valid loader binary to link to!` because
   `bin/` had `5.10.1/` and `5.11.1/` but no `5.11.2/`.

## Conclusion

The SDK is **not** installed as a mod SDK until its matching loader binaries are installed.
`geode sdk install-binaries` (non-interactive, one run, `Installing binaries for 5.11.2 … Done`)
created `bin/5.11.2/`, and after that the probe configured, compiled, linked and packaged:
`build/RelWithDebInfo/mh.probe.dll` and `build/mh.probe.geode`. The first build after
installing the binaries still exited 1 during packaging; an unchanged re-run completed with
`| Done | Successfully packaged mh.probe.geode`, so treat a first-run packaging failure as a
"configure dirty, build again" case rather than a defect.

So the observer is buildable on this machine with no user action: MSVC 14.44.35207 (VS 2022
BuildTools), Windows SDK 10.0.26100, Codegen downloaded per build, and a CMakeLists that adds
`$ENV{GEODE_SDK}` as a subdirectory. `harness/observer/` can now be written for real, and its
build output stays untracked.

`geode new` being interactive-only remains true, so `harness/observer/` carries its own
`mod.json` and `CMakeLists.txt` written to this recipe.

## Open questions

- **Does the observer link the portable core under MSVC?** The core has only been built with
  g++; O2 compiles `harness/src/*.cpp` into the mod, which is the real test. If a unit fails
  under MSVC, fixing it must not break the g++ tests.
- **Is `bin/5.11.2` complete for linking?** The probe links and packages, but it does not
  exercise a hook or a Geode symbol beyond `$on_mod`; O3 is the proof.
- **Packaging needs `mod.json` to be valid enough for the CLI's install step.** `DONT_INSTALL`
  keeps the probe out of every profile; the observer must do the same until the user installs
  it into `runtime511`.
