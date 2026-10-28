# Portable build of the reconstructed core

- **Question** — does `reconstruction/` compile from one source tree under a toolchain other
  than the MSVC build used so far, or is the core quietly tied to Windows?
- **Baseline** — commit `32751e6`, Mega Hack v9.1.3 reconstruction tree.
- **Date** — 2026-10-23.

## Method

The tree was configured and built out of source, with the build directory kept outside the
repository so nothing generated can be committed:

```powershell
cmake -G Ninja `
  -DCMAKE_CXX_COMPILER=<mingw64>/bin/g++.exe `
  -DCMAKE_BUILD_TYPE=Release `
  -S reconstruction -B <temp>/mh-build
cmake --build <temp>/mh-build
<temp>/mh-build/test_reconstruction.exe
```

| Component | Version |
| --- | --- |
| CMake | 4.4.4 |
| Generator | Ninja |
| Compiler | MinGW-W64 x86_64-ucrt-posix-seh g++ 16.2.0 (Brecht Sanders r2) |
| C++ standard | C++20 (`CMakeLists.txt` sets it, no per-target override) |

## Result

All ten build steps succeeded, including both libraries and the test executable:

```
[8/10] Linking CXX static library libhackpro_core.a
[9/10] Linking CXX shared library megahack.skeleton.dll
[10/10] Linking CXX executable test_reconstruction.exe
```

`test_reconstruction.exe` then ran all nine checks and exited `0`:

```
ok    geodeImplicitEntry runs and caches mod pointer
ok    module entry caches mod, runs async task, and drives early-load callbacks
ok    key/tag interning and round-trip resolution
ok    all 5 exported config var types (bool, double, int64, string, bytes)
ok    typed change listeners and hack listeners dispatch correctly
ok    action listeners, definitions, and safe_mode are functional
ok    settings registry JSON serialization produces valid hackpro.json schema
ok    settings registry JSON round-trips all five value types
ok    boolean setter intercepts action tags and leaves stored state untouched
```

## Platform surface actually used

A scan of every source, header and test for `windows.h`, `__declspec`, `_WIN32`,
`LoadLibrary` and `dlopen` returns exactly one site:

| File | Lines | Content |
| --- | --- | --- |
| `reconstruction/include/hackpro/types.hpp` | 10–20 | `#if defined(_WIN32) || defined(__CYGWIN__)` selects `__declspec(dllexport/dllimport)`; the `#else` branch uses `__attribute__((visibility("default")))` |

No source calls a Windows API, and no translation unit includes `windows.h`. The Geode
surface is a stand-in interface (`include/geode/entry.hpp`) rather than a vendored SDK, so
the core has no loader dependency at compile time. This MinGW build takes the non-MSVC side
of the only platform branch, which is why it is useful evidence and not just a second
compiler run.

## What this proves, and what it does not

**Proved:** the reconstructed core has no MSVC-only construct and no Windows SDK type in the
files that make up `hackpro_core`; it compiles clean under a GCC 16/C++20 front end and its
behavioural tests pass there.

**Not proved:** an Android/ARM64 or Linux build. MinGW still targets Windows, so this does
not exercise the pointer-width, endianness or ABI assumptions that a real cross-platform
build would. The M5 exit criterion "cross-platform implementation compiles from one source
tree" therefore stays open; the next step is an NDK/aarch64 compile of `hackpro_core`, which
needs the Android toolchain and cannot be done from this repository alone.

## Reproducing

The compiler path is machine-specific and deliberately not committed. On a machine with any
C++20 compiler, the same two commands with that compiler produce the same targets. The build
directory must stay outside `reconstruction/`; a stray `CMakeCache.txt` or object tree inside
it would be untracked build output.

### The preset, and why a bare configure fails (2026-10-28)

`reconstruction/CMakePresets.json` now commits the one configuration detail that is not
machine-specific: the compiler *name*. `cmake --preset mingw` asks CMake to resolve `g++` from
`PATH`, so it needs no absolute path, and it pins `Ninja`, `Release` and a git-ignored
`reconstruction/build` tree. That last detail is deliberate: `scripts/env_check.py` reports a
built test binary at `reconstruction/build/test_reconstruction.exe`, so following the preset is
what turns its reconstruction line green (`8 passed, 0 warning(s)` after this pass).

A bare `cmake -S reconstruction -B <dir> -G Ninja` fails on this machine, and the failure is
worth recording because it looks like a source problem:

- CMake 4.4 detects **MSVC 19.44.35229.0** (`...\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe`)
  and reports `Check for working CXX compiler: ... - broken` at `CMakeLists.txt:2 (project)`.
- The compile line it builds is `cl.exe /nologo /TP -D_MBCS /DWIN32 /D_WINDOWS /EHsc ...`; it
  fails because this shell has no `vcvars` environment, so `cl.exe` cannot find its own headers
  and libraries. Nothing in `reconstruction/` is at fault.
- Once a build directory has cached that choice, overriding `-DCMAKE_CXX_COMPILER` in the
  *same* directory is not enough: the cached MSVC platform flags reappear and MinGW's `g++.exe`
  is handed `/DWIN32` and `/EHsc` as if they were file names. Use a fresh directory, or the
  preset, whose `binaryDir` is fixed.

Verified this pass with the preset, on the toolchain in the table above:

```
-- Configuring done (2.5s)
[8/10] Linking CXX static library libhackpro_core.a
[9/10] Linking CXX shared library megahack.skeleton.dll
[10/10] Linking CXX executable test_reconstruction.exe
ok    boolean setter intercepts action tags and leaves stored state untouched
All core infrastructure reconstruction tests passed successfully!
```

The same tree also builds without CMake at all (`g++ -std=c++20 -O2 -I reconstruction/include`
over the four translation units), which is how the nine checks were re-confirmed before the
preset was added; the preset is the reproducible route, the direct compile is the fallback.

## Open questions

- UNVERIFIED: whether `hackpro_core` compiles for aarch64 — needs an NDK or a
  cross-compiling GCC/Clang, neither present on this machine.
- UNVERIFIED: behaviour on device; still blocked on M5's on-device criterion.
