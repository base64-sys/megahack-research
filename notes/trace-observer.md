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

## Conclusion

**The tree at `GEODE_SDK` is the Geode loader source, not a mod SDK.** `Geode/Enums.hpp` is a
generated header: it exists nowhere under `C:\Users\Robert_\Documents\Geode`, and there is no
`codegen/` directory to produce it. `cmake/GeodeFile.cmake` links a `geode-sdk` target that
nothing in the tree defines, and `cmake/Platform.cmake` cannot be included from outside it.

So a mod cannot be compiled against the current `GEODE_SDK`, and the sanctioned fix is to
install the actual mod SDK with `geode sdk install <path>` (a distribution that ships the
generated headers and a self-contained CMake package), then point only the observer's build at
it — not the global CLI configuration, so the existing loader checkout and the reconstruction
build keep working.

`geode new` being interactive-only is a permanent constraint: `harness/observer/` must carry
its own `mod.json` and `CMakeLists.txt` in the repository, written to match the SDK's expected
layout once the SDK is available.

## Open questions

- **Does `geode sdk install` work non-interactively here?** Untested. If it prompts, the item
  is blocked on the user.
- **Which SDK version string to pin.** The loader is 5.11.2; the contract's runtime baseline is
  Geode 5.11.2, so the mod SDK must be 5.11.x. UNVERIFIED until the install is done.
- **Generated-header set.** Whether the SDK distribution's include tree is complete for
  `#include <Geode/Geode.hpp>` on this toolchain is unknown until a probe compiles again.
