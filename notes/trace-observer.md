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

## Hook bridge (O3)

One hook, end to end, as of iteration 4:

| Field | Value | Evidence |
| --- | --- | --- |
| hook | `PlayLayer::resetLevel` | `notes/address-map.md:50`; contract example trace uses the same key |
| key | `GeometryDash.exe:rva:0x3b8eb0` | contract §hook event |
| rva | `0x3B8EB0` | address-map row; image base 0x140000000 |
| mechanism | Geode `$modify`, so the loader resolves the address | `Geode/modify/PlayLayer.hpp` |
| call ids | `c1`, `c2`, … per session; depth from an explicit stack | contract §call pairing |

The detour writes `enter` before calling the original and `exit` after it, with
`call.duration_ns` on the exit. `args` is `unavailable` (this hook does not capture them) and
`return` is `null` for a void method; neither is invented. Events go through
`mh::TraceWriter::write_event`, and the file stays open for the run: `meta` at load, `end` on
`$on_game(Exiting)`, which the SDK documents as the safe place for global cleanup.

UNVERIFIED: that the hook fires at all, and that the thread running `$on_mod(Loaded)` is the
game's main thread. The record says `role: main` only when the hook runs on that same thread,
and the code carries the same caveat. A captured run closes both.

## Open questions

- **Does the observer link the portable core under MSVC?** Yes as of iteration 3: the mod
  compiles `harness/src/*.cpp` into `megahack.trace.dll` with MSVC 14.44, no source changes,
  and the g++ suite stays at 162/162.
- **Where do traces live?** The skeleton writes to `Mod::get()->getSaveDir()/"traces"/`
  because that is the one location a mod may write without assuming an install layout. O4
  decides the path the runner reads; if it must be outside the mod's save directory, the
  observer changes, not the runner's expectations.
- **Packaging exit code.** The first build after a configure change returns 1 while still
  producing the `.geode`; an unchanged re-run returns 0. Treating "exit 1 with the package
  written" as success would be a claim without a boundary, so the loop rebuilds instead.
- **Is `bin/5.11.2` complete for linking?** The observer links and packages; a hook is still
  the proof, and that is O3.
