# Trace observer build progress

Backlog and evidence for the runtime half of `megahack.trace`. Instructions:
[OBSERVER-LOOP.md](OBSERVER-LOOP.md). Durable findings: [trace-observer.md](../notes/trace-observer.md).
The host-testable core this builds on: [trace-harness.md](../notes/trace-harness.md).

Status vocabulary: `pending`, `in progress`, `done`, `blocked`.

## Backlog

| id | item | status |
| --- | --- | --- |
| O1 | Discover the Geode 5.11.2 mod build path: SDK layout, exact command, produced artifacts, host toolchain | done |
| O2 | Observer skeleton: mod `megahack.trace`, links the portable core, writes meta + end to its save directory (`traces/`) | done (build only — the write is unproven until a run) |
| O3 | Hook bridge: enter/exit events with call ids, depth and proven main-thread role | pending |
| O4 | Runner: hashes, one-target enforcement, sha256 sidecar, runtime511 selection, manifest feeding | pending |
| O5 | Pilot capture in runtime511 and a real `read_trace` pass (needs the user to launch) | pending |

## Evidence log

### Iteration 1 — O1, first half: the current SDK cannot build a mod

- Probed with a throwaway project in `%TEMP%` (not committed); every failure is recorded in
  [trace-observer.md](../notes/trace-observer.md) §What was tried.
- Established: `geode` CLI 3.9.0; `geode build` drives CMake + MSVC (VS 2022 BuildTools, cl
  14.44.35207, Windows SDK 10.0.26100); `geode new` is interactive-only; `GEODE_SDK` is the
  Geode **loader source** (5.11.2) and lacks the generated `Geode/Enums.hpp`, so no mod
  compiles against it; `cmake/Platform.cmake` cannot be included out of tree and
  `setup_geode_mod` needs a `geode-sdk` target the tree does not define.
- Next: install a real mod SDK with `geode sdk install <path>` into a dedicated directory and
  re-probe, pointing only the observer build at it. If the install is interactive, O1 becomes
  blocked on the user.
- Commit: `docs(observer): record the Geode 5.11.2 mod build path probe`.

### Iteration 2 — O1 done: the SDK builds a mod once its loader binaries exist

- `geode sdk install <path>` refuses to install a second SDK; `--reinstall` would uninstall
  the existing checkout, so it was not used.
- `geode sdk install-binaries` (non-interactive) added `bin/5.11.2/`; without it configure
  dies at `CMakeLists.txt:403` with "No valid loader binary to link to!".
- The working project shape is `add_subdirectory($ENV{GEODE_SDK} ...)` then
  `include($ENV{GEODE_SDK}/cmake/GeodeFile.cmake)` then `setup_geode_mod(<target> DONT_INSTALL)`;
  the SDK fetches CPM deps and downloads `Codegen.exe` itself.
- Evidence: probe produced `build/RelWithDebInfo/mh.probe.dll` and `build/mh.probe.geode`
  with MSVC 14.44.35207 and Windows SDK 10.0.26100. Nothing was installed into a profile.
- Commit: `docs(observer): record the working Geode mod build recipe`.
- Next: O2, the observer skeleton against this recipe.

### Iteration 3 — O2: the observer mod builds, and the core is MSVC-clean

- `harness/observer/` now holds `mod.json` (id `megahack.trace`, geode 5.11.2, gd 2.2081),
  `CMakeLists.txt` (the recipe above, `DONT_INSTALL`) and `src/main.cpp`.
- `src/main.cpp` writes one meta and one end record on `$on_mod(Loaded)` through
  `mh::TraceWriter`. Everything it cannot know at load (target hash, scenario, settings,
  level, GD version) is null *and* named in `capture.missing` with a reason, so the trace is
  honestly ineligible as evidence rather than quietly wrong.
- **The portable core compiles and links under MSVC 14.44 with no source changes** — the
  g++/ctest suite is untouched and still 162/162. That closes one open question from
  iteration 2.
- Artifacts: `harness/observer/build/RelWithDebInfo/megahack.trace.dll` and
  `build/megahack.trace.geode`, both gitignored (`/harness/observer/build/`).
- The first build after a configure change exits 1 during packaging while still writing the
  `.geode`; an unchanged re-run exits 0. Recorded, not worked around.
- `check_doc_links.py` failed because it walked the CPM dependency READMEs under
  `build/_deps`; `build` and `_deps` are now in `SKIP_DIRS`, with a test that a vendored
  README under `build/` cannot fail the gate.
- Not proven yet: that the mod actually writes the file in game. That needs a launch, which
  belongs to O5.
- Commit: `feat(observer): add the megahack.trace observer skeleton`.
- Next: O3, the hook bridge.
