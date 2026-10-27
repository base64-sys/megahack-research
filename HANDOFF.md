# Handoff: continue the Mega Hack runtime-validation work

Paste this whole file (or point the new session at it) as the opening message. It assumes a
fresh context and no memory of the previous session.

## What this repository is

`C:\Users\Robert_\Documents\megahack-research-new-codebase` - a reverse-engineering and
preservation workspace for Mega Hack v9.1.3 (Geode mod `absolllute.hackmega`) running on
Geometry Dash 2.2081 / Geode 5.11.2.

**Read `AGENTS.md` first and treat it as binding.** The parts that matter most here:

- Ground truth is `samples/absolllute.hackmega/absolllute.hackmega_original.dll` (8,697,056
  bytes, SHA-256 `1635c339…e1107a92`, image base `0x180000000`). `samples/` and `recovered/`
  are evidence: read, never edit, never commit.
- Evidence or it does not exist: every address, offset and claim carries file, VA and the tool
  that produced it. Verify twice. Label `UNVERIFIED: <what would close it>` instead of
  guessing.
- Never commit binaries, assets, traces or user data. Never mark a roadmap criterion complete.
- Python is `C:\Users\Robert_\AppData\Local\Python\bin\python3.exe` (`python` is a Store stub).

## Where the work stands

The runtime-validation harness is **built and working end to end on the release side**.

1. **Trace core** (`harness/`): dependency-free C++20 static library `mh_trace` plus its own
   test runner - contract, value model, canonical JSON, SHA-256, fingerprint, trace writer,
   call-stack validation, strict JSON parser, trace reader, scenario manifest parser,
   comparator, sequence alignment, comparison and statuses. **196 test cases, 6 ctest entries,
   all green.** Design notes: `notes/trace-harness.md`.
2. **Observer** (`harness/observer/`): Geode mod `megahack.trace` v0.1.1 that records real
   traces. Notes: `notes/trace-observer.md`.
3. **Runner** (`harness/tools/`): `preflight.py` (hashes, one-target rule, profile guard, run
   context, settings tags), `grade_trace.py` (grade the newest capture), `mh_scenario_check`,
   `mh_trace_compare`, `mh_trace_check`.
4. **Real captures**: evidence-grade runs exist in
   `%LOCALAPPDATA%\GeometryDash\geode\mods\megahack.trace\traces\`. A complete one looks like:
   `capture: complete`, target `1635c339…`, scenario `d8505f08…` (`safe-mode-smoke`, draft),
   35k-54k events, 0 errors/warnings, every hook naming `absolllute.hackmega.dll` as the owner.

**The comparison is `blocked`, correctly**, on exactly one thing: there are two release runs but
**no reconstruction variant at all**. `mh_trace_compare` prints one finding: "at least two runs
per variant are required".

## The next task: a reconstruction mod

`reconstruction/` is a host-testable core plus a `geode/entry.hpp` shim. It has **no `$modify`
hooks and produces no loadable `.geode`**, so it cannot emit traces. It needs to become a Geode
mod that:

1. has its own mod id (e.g. `megahack.reconstruction`) and its own `CMakeLists.txt`, copying the
   observer's verified build recipe (below);
2. installs the same hook surface: `0x3b8eb0` `PlayLayer::resetLevel`, `0x3b4ba0`
   `PlayLayer::postUpdate`, `0x397f40` `PlayerObject::pushButton`, `0x3981d0`
   `PlayerObject::releaseButton`, plus the `level_started` / `attempt_started` milestones;
3. emits through the shared `mh_trace` core (link the same sources; do not fork the contract);
4. is packaged and run with `--role reconstruction`, so `hook.target` names a different DLL and
   the gate has two real variants.

Do **not** run both target mods at once: one target per profile, one target per run.

## Verified commands

Build and test the core:

```
cmake --build harness/build
ctest --test-dir harness/build --output-on-failure      # 6 entries, 170 cases
```

Build the observer (needs the Geode SDK at `$env:GEODE_SDK=C:\Users\Robert_\Documents\Geode`):

```
cd harness\observer
geode build        # first build after a change exits 1 while packaging; re-run gives 0
```

Grade the newest capture, and compare:

```
python harness\tools\grade_trace.py --latest
harness\build\mh_trace_compare.exe harness\scenarios\safe-mode-smoke.json --release <trace.jsonl>
```

Gates before committing: `python scripts/check_doc_links.py`,
`python scripts/make_progress_svg.py --check`, `python harness/tools/test_preflight.py`,
`python harness/tools/test_grade_trace.py`, and ctest.

## Run book (the parts that cost time to learn)

- **The mod build recipe**, since `geode new` needs a TTY: a mod project is
  `add_subdirectory("$ENV{GEODE_SDK}" "${CMAKE_BINARY_DIR}/geode-sdk")`, then
  `include("$ENV{GEODE_SDK}/cmake/GeodeFile.cmake")`, then
  `setup_geode_mod(<target> DONT_INSTALL)`. `geode sdk install-binaries` must have created
  `bin/5.11.2/` or configure dies at `CMakeLists.txt:403`.
- **Geode loads `.geode` archives only.** A folder in `mods/` fails to load in game.
  `geode package new <dir> -o <outside-the-repo>.geode`; without `-o` it writes into `samples/`.
- **Mod data lives in `%LOCALAPPDATA%\GeometryDash\geode\mods\<id>\`**, not in the profile.
- **Run contexts are single use.** `preflight.py --context <path>` writes one; the observer
  consumes it and renames it `.consumed`. It looks for `MH_TRACE_CONTEXT` and, failing that,
  `pending-context.json` in its own data directory. A `partial` capture usually means the context
  was already consumed, or the installer/build changed underneath it.
- **Profile**: use only `runtime511` (`geode profile run runtime511`). The default `a` profile is
  the user's Steam install and is off limits; `preflight.py` refuses it.
- **Settings tag 482** (`HACK_SAFE_MODE`) is a Map B entry: its value is membership in
  `PROFILES[PROFILE]` of `hackpro.json`, so pass
  `--tag-key 0x1E2=flag:CHEAT_SAFETY/HACK_SAFE_MODE`. Verified by observation.
- **The installed target must be the release.** `--expect-sha256 1635c339…` pins it; the sample
  folder holds four distinct builds of identical size.

## What is deliberately not done

- `savePercentage` (`0x16c8b0`) is hooked but has never fired in two sessions, including one
  completion, while the game did write its save files. It is an *optional* key in the manifest;
  recorded as an open question rather than chased further.
- No `pass` has ever been produced, and none should be until two variants × two runs exist.
  `notes/trace-observer.md` explains what each status needs.

## Open questions worth reading before designing anything

`notes/trace-observer.md` §"What still blocks a comparison" and §"Hardening the observer", plus
the open questions at the end of `notes/trace-harness.md`.
