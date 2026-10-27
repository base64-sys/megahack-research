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
| O3a | Hook bridge, one hook end to end: `PlayLayer::resetLevel` enter/exit with call ids, depth, parent and `thread.role` from the load thread | done (build only — firing unproven until a run) |
| O3b | Hook bridge, remaining pilot keys: `PlayLayer::postUpdate`, `PlayerObject::pushButton`, `PlayerObject::releaseButton` | done (build only) |
| O4a | Runner preflight: target/environment hashes, one-target rule, profile refusal, trace sha256 sidecar, fragment in meta spelling, python tests | done |
| O4b | Run-context handshake: the runner writes a context file before launch, the observer fills meta (target, environment, scenario, level, settings) and the real `run_fingerprint` from it at load, then marks it consumed | pending |
| O5 | Pilot capture in runtime511 and a real `read_trace` pass, plus the proofs only a run can give (hooks fire; the load thread is the main thread) - needs the user to launch | pending |

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

### Iteration 4 — O3a: the first hook emits real enter/exit pairs

- `PlayLayer::resetLevel` at RVA `0x3B8EB0` (notes/address-map.md:50, and the contract's own
  example trace names `GeometryDash.exe:rva:0x3b8eb0`). Hooked with `$modify`, so Geode
  resolves the address; the record still states the RVA and the runtime VA.
- The detour writes `hook` enter before the original and `hook` exit after it, with
  `call_id` from a per-session counter, `depth` from an explicit stack, `parent_call_id`
  from that stack, and `call.this` as an observed `ptr` value. `args` is `unavailable` with a
  reason and `return` is the contract's `null` - both honest for a void method whose
  arguments this hook does not capture.
- The session now stays open for the whole run: `meta` at load, events as they happen, `end`
  on `$on_game(Exiting)` (the SDK documents that event as the place for global cleanup). A
  crash therefore produces no `end`, which the reader already treats as incomplete.
- Built and packaged: `megahack.trace.dll`, `megahack.trace.geode`, exit 0 on a clean rerun.
- **Unverified until a run**: that the hook fires, and that the load thread is the game's
  main thread. Both are named in the note and in the code.
- Commit: `feat(observer): hook PlayLayer::resetLevel and emit enter/exit pairs`.
- Next: O3b, the remaining pilot keys.

### Iteration 5 — O3b: all four required pilot hooks are in

- The pilot's four required keys (`notes/runtime-validation.md` §hooks, manifest
  `safe-mode-smoke`) are now implemented, each verified by two angles:

  | RVA | Method | Note evidence | Geode 2.2081 binding |
  | --- | --- | --- | --- |
  | `0x3B8EB0` | `PlayLayer::resetLevel` | address-map.md:50 | `modify/PlayLayer.hpp:1150` |
  | `0x3B4BA0` | `PlayLayer::postUpdate` | audio-sync-feature.md:103 | `modify/PlayLayer.hpp:1120` |
  | `0x397F40` | `PlayerObject::pushButton` | gameplay-hooks.md:1590 | `modify/PlayerObject.hpp:1925` |
  | `0x3981D0` | `PlayerObject::releaseButton` | gameplay-hooks.md:1591 | `modify/PlayerObject.hpp:1931` |

  The binding column is generated code shipped with the SDK, so the notes and the loader
  agree on the addresses - the constants in the observer are not a lone transcription.
- The hook plumbing is now shared: `HookScope begin_hook(spec, this)` / `end_hook(...)`, so a
  new key is a spec constant plus a two-line detour. The stack pop also recovers if a
  detour returned out of order (it removes its own id instead of assuming the top).
- `savePercentage` (`0x16C8B0`) stays out: the manifest lists it as optional and UNVERIFIED.
- Not proven yet, and now tracked in O5: that these hooks fire, and that the load thread is
  the main thread. A build cannot answer either.
- Commit: `feat(observer): hook the four required pilot methods`.
- Next: O4, the runner.

### Iteration 6 — O4a: preflight refuses what must not run, and hashes what must

- `harness/tools/preflight.py` + 7 tests (`harness/tools/test_preflight.py`, all passing).
  It refuses the live `a` profile, any GD path under `steamapps`, zero or two targets, and a
  missing artifact; it hashes target/GD/Geode/observer and emits the contract's exact
  `target`/`environment` objects, so there is no second schema.
- Run against the real files, read-only: target `1635c339…e1107a92`, size `8697056`, image
  base `0x180000000` (all match AGENTS.md), GD `fc5a16c2…` (matches the documented
  2.2081 image), Geode `0c34f504…`.
- `--sidecar <trace>` writes sha256sum-form `<trace>.sha256`.
- The item grew a second half, so it is split: the observer still writes `null` for what only
  the runner knows, and patching a trace's meta afterwards would be rewriting evidence.
  O4b therefore makes the runner write a **run-context file** before launch and the observer
  read it at load - meta is written once, with real values, and `run_fingerprint` becomes
  computable instead of all-zero.
- Commit: `feat(runner): preflight hashes, one-target rule and profile refusal`.
- Next: O4b, the run-context handshake.
