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

All four keys the pilot marks required are wired (iteration 5):

| RVA | Method | Notes | Geode 2.2081 binding |
| --- | --- | --- | --- |
| `0x3B8EB0` | `PlayLayer::resetLevel` | address-map.md:50 | `modify/PlayLayer.hpp:1150` |
| `0x3B4BA0` | `PlayLayer::postUpdate` | audio-sync-feature.md:103 | `modify/PlayLayer.hpp:1120` |
| `0x397F40` | `PlayerObject::pushButton` | gameplay-hooks.md:1590 | `modify/PlayerObject.hpp:1925` |
| `0x3981D0` | `PlayerObject::releaseButton` | gameplay-hooks.md:1591 | `modify/PlayerObject.hpp:1931` |

`GJGameLevel::savePercentage` (`0x16C8B0`, address-map.md:52) is optional in the manifest and
stays unhooked until the pilot's trigger question is answered.

The detour writes `enter` before calling the original and `exit` after it, with
`call.duration_ns` on the exit. `args` is `unavailable` (this hook does not capture them) and
`return` is `null` for a void method; neither is invented. Events go through
`mh::TraceWriter::write_event`, and the file stays open for the run: `meta` at load, `end` on
`$on_game(Exiting)`, which the SDK documents as the safe place for global cleanup.

UNVERIFIED: that the hook fires at all, and that the thread running `$on_mod(Loaded)` is the
game's main thread. The record says `role: main` only when the hook runs on that same thread,
and the code carries the same caveat. A captured run closes both.

## Runner preflight (O4a)

`harness/tools/preflight.py` collects what the observer cannot know at load and emits it as
the exact `target` and `environment` objects of a meta record - no new schema, no second
spelling:

```
python harness/tools/preflight.py --target <artifact> --gd <GD exe> --geode <Geode.dll>
    --harness <megahack.trace.geode> --profile runtime511 --gd-version 2.2081
    --geode-version 5.11.2 [--out fragment.json] [--sidecar <trace>]
```

It refuses a run when the profile is the live `a`, when the GD path is under `steamapps`,
when there is not exactly one target artifact, and when an artifact is missing. `--sidecar`
writes `<trace>.sha256` in sha256sum form.

Run against the real files on 2026-10-27 (read-only; nothing written into the profile):

| Field | Value | Matches |
| --- | --- | --- |
| target sha256 | `1635c339…e1107a92` | AGENTS.md ground truth |
| target size | `8697056` | AGENTS.md ground truth |
| target image_base | `0x180000000` | AGENTS.md ground truth |
| GD sha256 | `fc5a16c2…d9ec6d0` | AGENTS.md GD 2.2081 prefix `FC5A16C2…C6D0` |
| Geode sha256 | `0c34f504…33684d3` | not previously recorded |

Tests (`python harness/tools/test_preflight.py`, 7 cases) cover the digest against
`hashlib`, the PE header read, the sidecar format, both profile refusals, the one-target rule
(0, 1, 2 and missing), the fragment's field names, and the CLI's refusal exit code.

## Run context (O4b)

The runner must not patch a trace after the fact - that would turn a contemporaneous record
into a reconstruction. Instead it hands its facts over **before** launch:

- `MH_TRACE_CONTEXT=<path>` is set on the game process.
- The file holds the meta body (`target`, `scenario`, `environment`, `settings`, `level`,
  optionally `mods`/`clock`) plus `repetition_index`, which is the sixth fingerprint input and
  is deliberately not part of the meta record.
- At load the observer parses it (`mh::read_run_context`, `harness/src/context.cpp`), splices
  the body into `meta`, computes `run_fingerprint` with the core's own implementation, and
  renames the file to `<path>.consumed` so a stale context cannot silently apply to a second
  run.
- No context, or a rejected one: the observer falls back to the null body with
  `capture.missing` explaining every field, and the trace stays ineligible - which is what a
  build without a runner should produce.

`harness/tools/preflight.py --scenario <manifest> --repetition N --context <path>` writes it,
so the fragment and the context come from one place. `read_run_context` requires the same
five members the trace reader requires of a meta record, a non-negative integer
`repetition_index`, `settings.tags` as an array, and four 64-lowercase-hex digests
(`target`, `scenario`, `environment.harness`, `level`) - a placeholder cannot enter a
fingerprint by accident.

## Validator CLI (O4c)

`mh_trace_check <trace.jsonl> [--json]` runs the core reader over a file, so a captured run
can be graded without writing a test:

| Situation | Exit | Output |
| --- | --- | --- |
| note's seven-line example | 0 | `valid, 5 event(s), 0 error(s), 3 warning(s)`, meta and end present |
| same file with the `end` line removed | 1 | `missing_end: no end record: the trace is incomplete` |
| no argument | 2 | usage |
| `--json` | as above | one object: `structurally_valid`, `meta`, `end`, `events`, `errors`, `warnings`, `issues[]` |

The warnings on the example are the reader's own and are expected: `meta` omits `mods`, the
`end` record omits `artifacts`, and one event's capture is `unavailable`. ctest covers the
first and third rows (`trace_check_example`, `trace_check_usage`); the fixture is
`harness/tests/fixtures/example_trace.jsonl`, written by the same generator as the header.

## Milestones and frames (O6)

| Record | Source | Address |
| --- | --- | --- |
| `milestone level_started` | `PlayLayer::init`, emitted before the original | RVA `0x3A5B00` (hook-registrations.md:117; binding note) |
| `milestone attempt_started` | `PlayLayer::resetLevel`, at entry | RVA `0x3B8EB0` (address-map.md:50) |
| `frame` on every event | `CCDirector::getTotalFrames()` | cocos `CCDirector.h:179` |

`PlayLayer::init` is deliberately *not* a hook key: the pilot lists four hook keys and init is
not among them, so the milestone is the record for it and no key is invented.

`level_started` is written before the original `init` runs, because setting a level up can
call `resetLevel` itself and the manifest's order group requires `level_started` first. The
cost is that a failing `init` still records the attempt; that is the honest reading of "a
level was started", and the trace shows no later events for it.

## Settings snapshot (O7)

`settings.snapshot_sha256` is a file hash, so the runner can always produce it. `settings.tags`
is a claim about state, so it is only written when it was **read**, never copied from the
manifest:

```
python harness/tools/preflight.py ... --scenario <manifest> --settings <hackpro.json>
    --tag-key 0x1E2=<stored key> --context <path>
```

- The manifest says what a tag must be; the file says what it is. A mismatch refuses the run.
- `tag_keys` exists because the file stores **names** and the number↔name join happens at
  runtime: `hackpro::extensions::tagFor` (`0x180072DF0`) resolves a name through the map built
  by `intern` (`0x18007C700`) from the file itself (`notes/settings-contents.md`, lines
  309-318). Static analysis cannot supply the mapping, so the operator supplies it and the
  runner verifies the value.
- A key present in two value groups (e.g. `V_BOOL` and `V_INT`) is refused: two values for one
  key has no single interpretation.

**Where tag 482 actually lives.** `safe-mode-feature.md` reads the gate as
`isHackEnabled(HACK_SAFE_MODE = 482)` - a **Map B** entry, not a `V_BOOL` key. Map B is stored
as the active profile's enabled list: `PROFILES["DEFAULT"]` is an array of `SECTION/HACK_NAME`
strings (`CHEAT_SAFETY/HACK_SAFE_MODE_POPUP`, `BYPASS/HACK_ANTI_KICK`, ...), and a name that is
absent is off. `CHEAT_SAFETY` is a real section id in the release's own string table
(`absolllute.hackmega_original.dll`, file offset `0x533a01`, in the list
`...COSMETIC.LEVEL.STATUS.UNIVERSAL.CHEAT_SAFETY.DISPLAY.UTILITY...`).

So `preflight.py` accepts `--tag-key <tag>=flag:<SECTION/NAME>` for Map B and reads membership:

```
--tag-key 0x1E2=flag:CHEAT_SAFETY/HACK_SAFE_MODE
```

**What the files say.** Two settings files exist on this machine: the repo sample
(`.oc/hackpro.json`, 256 entries) and the live one written by the capture run
(`%LOCALAPPDATA%\GeometryDash\geode\mods\absolllute.megahack\v9\hackpro.json`, 256 entries).
They differ only in three enabled names; **neither contains
`CHEAT_SAFETY/HACK_SAFE_MODE`**, so Safe Mode is off in both. Running the real command against
the live file with the pilot manifest refuses at the gate the record needs:

```
preflight refused: tag 0X1E2: the file has False but the scenario declares True
```

That refusal is the feature working: the record would otherwise claim an observation the
profile contradicts.

**Observed, not inferred (2026-10-27).** Safe Mode was toggled on in the mod, the game was
quit, and the enabled list went from 53 to 54 entries with `CHEAT_SAFETY/HACK_SAFE_MODE`
present. That closes the mapping: tag 482 is stored as that profile-list entry, and the runner's
`--tag-key 0x1E2=flag:CHEAT_SAFETY/HACK_SAFE_MODE` now reads it as `true`.

## Target identity: the installed build is not the release (2026-10-27)

The trace above was produced against a target whose DLL does not match the ground truth. Four
distinct builds exist in `samples/absolllute.hackmega/`, all 8,697,056 bytes, so only a digest
separates them:

| File | SHA-256 |
| --- | --- |
| `absolllute.hackmega.dll` (what was installed) | `24CEBCAC…AA47996` |
| `absolllute.hackmega_original.dll` (ground truth) | `1635C339…E1107A92` |
| `absolllute.hackmega_pre_persistence.dll` | `3B116767…5FED0061` |
| `absolllute.hackmega_working_aslr.dll` | `3B448741…A47996` |

The installed copy
(`GD-Runtime-511\geode\unzipped\absolllute.hackmega\absolllute.hackmega.dll`) hashes to
`24CEBCAC…`, i.e. it is a fourth build, not the release and not either named experiment. No
false claim was written: those traces carry no `target` at all (no run context), `capture` is
`partial`, and the fingerprint is zeros.

So a release trace needs the release installed, and `--target` must name the artifact the game
actually loads - the unzipped copy under the profile - not the sample it was packaged from.
A release package was built from `_original.dll` and verified to contain
`1635C339…E1107A92`:

```
geode package new <temp copy with _original.dll as absolllute.hackmega.dll> -o <temp>.geode
```

## First captured run (2026-10-27, `runtime511`)

The observer was installed into the isolated profile and launched by the user. The log shows
the mod loading and Geode enabling all five hooks, at a runtime base of `0x7FF675F50000`:

```
Enabled PlayLayer::postUpdate hook at 0x7ff676304ba0 for megahack.trace
Enabled PlayLayer::resetLevel hook at 0x7ff676308eb0 for megahack.trace
Enabled PlayLayer::init hook at 0x7ff6762f5b00 for megahack.trace
Enabled PlayerObject::pushButton hook at 0x7ff6762e7f40 for megahack.trace
Enabled PlayerObject::releaseButton hook at 0x7ff6762e81d0 for megahack.trace
```

Each runtime address is the base plus the RVA in this note, which is the second angle the
build could not give: `0x7FF675F50000 + 0x3B8EB0 = 0x7FF676308EB0`.

The trace was written to
`%LOCALAPPDATA%\GeometryDash\geode\mods\megahack.trace\traces\<run_id>.jsonl` - **not** into
the profile folder. Geode 5 redirects mod data to `%LOCALAPPDATA%`, so the runbook's old path
was wrong and is corrected below.

`mh_trace_check` on that file: `valid, 26 event(s), 0 error(s), 0 warning(s), meta present,
end present`. Contents: 13 `pushButton` enter/exit pairs, `thread.role: main` on every event,
frames 579..12260, depths all 1, `counts.unclosed: 0`. No milestones and no `postUpdate`,
`resetLevel` or `init` records, which matches a session that never entered a level - the meta
is the null fallback, so `capture.status` is `partial` and the fingerprint is all zeros.

That answers two things the build could not: the hooks fire, and the thread running
`$on_mod(Loaded)` is the thread that runs gameplay. It does not yet answer whether
`level_started` precedes `attempt_started`, because no level was loaded.

## Second captured run: gameplay (2026-10-27, `runtime511`)

Run `a8c8814b-13b4-4cfa-8834-bc1a4588f1bd`, 13.4 MB of JSONL, graded by `mh_trace_check`:
`valid, 17097 event(s), 0 error(s), 0 warning(s), meta present, end present`.
`end.counts`: `events 17097, attempted 17097, dropped 0, unclosed 0, hook_enter 8541,
hook_exit 8541, errors 0`.

| Observation | Value |
| --- | --- |
| target mod loaded | `meta.mods` = `[megahack.trace, absolllute.hackmega]` |
| required hook keys, all four | `0x3b8eb0` 13/13, `0x3b4ba0` 8306/8306, `0x397f40` 75/75, `0x3981d0` 147/147 (enter/exit) |
| milestones | 2 × `level_started`, 13 × `attempt_started` |
| order group | `level_started` (frame 5275) precedes the first `attempt_started` (frame 5293) |
| thread | `main` for all 17097 events, one thread id |
| frames | 5275 .. 24497 |
| call depth | 1 for 16978 events, 2 for 104 - real nesting, not a flat log |

This is the evidence the build could not produce on its own. It confirms that the four hook
keys the pilot marks required do fire, that `level_started` really does precede
`attempt_started` in a run, that the observer's thread role is the gameplay thread, and that
enter/exit pairing holds with `unclosed: 0` across 8,541 pairs.

What it still is not: **evidence-grade**. `meta.capture.status` is `partial` with 14 missing
fields, because the run had no `MH_TRACE_CONTEXT` - no target hash, no scenario, no settings,
and an all-zero `run_fingerprint`. A comparison also needs two variants (release and
reconstruction) and the manifest's two repetitions per variant.

## Third captured run (2026-10-27, `runtime511`)

Run `5da34cbb-820b-4727-ad15-fcbd55790235` with Safe Mode enabled:
`valid, 16434 event(s), 0 error(s), 0 warning(s)`, `dropped 0`, `unclosed 0`, 8213 balanced
hook pairs, 2 `level_started` and 6 `attempt_started`, order group correct, `main` on every
event, frames 4700..23222, depths 1 and 2. The pipeline is now reproducible across runs.

The target identity check below is the reason this run is still not evidence about the release.

## Fourth captured run: evidence-grade (2026-10-27)

Run `661c34c2-b35d-47c3-acac-c8b56463ab24`, with `MH_TRACE_CONTEXT` pointing at a context
built against the release artifact:

```
valid, 8811 event(s), 0 error(s), 0 warning(s), meta present, end present
capture: complete (no missing fields)
run_fingerprint: 428e8e6e47d34355a6e81e8488c6b285d6448ba745008b46a59e4ea420ad6f76
```

| meta field | value |
| --- | --- |
| `target` | `absolllute.hackmega.dll`, `1635c339…e1107a92`, 8,697,056, `0x180000000`, role `release` |
| `scenario` | `safe-mode-smoke`, `06b35d09…` |
| `settings.tags` | `[{tag 0x1E2, HACK_SAFE_MODE, value true, source settings}]` |
| `settings.snapshot_sha256` | `bc941968…` |
| `mods` | observer `0.1.0` + target `9.1.3` (written by the observer from the live mod list) |
| counts | 8,811 attempted, 0 dropped, 0 unclosed, 4,403 balanced pairs |
| milestones | 1 `level_started`, 4 `attempt_started` |

That is the first trace this project can call evidence: real hashes, real settings, a
deterministic fingerprint, and nothing missing. Repetition 1 of the same context exists
(`ctx-run1.json`, `repetition_index: 1`).

## What still blocks a comparison

Two things, neither of them in the observer:

1. **The reconstruction cannot emit traces.** `reconstruction/` is a host-testable core plus a
   `geode/entry.hpp` shim; it has no `$modify` hooks and produces no loadable mod, so there is
   no second variant to compare against. It needs to become a Geode mod (own id, own package)
   that installs the same four hooks and two milestones through this harness.
2. **Nothing in a trace currently depends on the target.** The observer reports the hook it
   installed, and `hook.target` is always `null` - so a release trace and a reconstruction trace
   of the same GD session would align trivially. To make the comparison mean something, the
   trace has to carry target-dependent evidence: the target's own detour identity
   (`hook.target`: registration VA, callback VA, tag) and/or effect events such as `save`
   records for the `savePercentage` veto. That is the next design question, not a coding gap.

## `hook.target`: naming the implementation that was running (2026-10-27)

A trace whose only content is the observer's own GD hooks cannot tell releases apart - the
release and a reconstruction would hook the same four functions and align trivially. So the
observer now resolves, per hooked address, **which other loaded mod owns a hook there**, and
writes it as `hook.target`:

```cpp
for (Mod* mod : Loader::get()->getAllMods()) {          // every mod except the observer
    for (Hook* hook : mod->getHooks()) {                 // loader/Hook.hpp
        if (hook->getAddress() == address) {             // the same GD function
            module = mod->getID(); priority = hook->getPriority();
```

Resolved lazily on the first event for an address and cached, because the target's hooks may
not exist yet when the observer's own `$on_mod(Loaded)` runs.

Fields Geode 5.11.2 exposes are filled (`module`, `priority`); the rest of the contract's
`hook.target` object stays present-but-null rather than invented:

| Field | Value | Why |
| --- | --- | --- |
| `module` | the owning mod's id | `Mod::getID()` |
| `priority` | `Hook::getPriority()` | documented accessor |
| `callback_va`, `callback_rva` | null | the hook API exposes no detour address |
| `registration_va`, `tag`, `tag_label` | null | the target's own registration is internal to it |
| `order_raw`, `order_signed` | null | ditto; not the same thing as Geode priority |

This is the difference that matters for a comparison: a release run and a reconstruction run
over the same session produce the same keys and the same order, and differ in `hook.target`.
Still UNVERIFIED until a run is made: whether the release's `$modify` hooks are visible through
`Mod::getHooks()` at all (Geode tracks modify-generated hooks, but only a run proves it).

## Runbook for the first capture (needs the user)

Nothing below has been executed; installing a mod into a profile and launching a game are
user actions.

1. Build the target's package and install both mods into `runtime511`. Geode loads **`.geode`
   archives only**: a folder dropped into `mods/` produces an in-game error and fails to load
   (observed, 2026-10-27). The CLI can build the archive, but it must be pointed away from
   `samples/` or it writes into the evidence tree:
   `geode package new samples/absolllute.hackmega -o <somewhere outside the repo>\absolllute.hackmega.geode`
   then copy that archive and `harness/observer/build/megahack.trace.geode` into the profile's
   `mods/` directory. Check the target's DLL hash first: all four sample DLLs are 8,697,056
   bytes, so only the digest distinguishes the release from the experiments.
2. Write the context:
   `python harness/tools/preflight.py --target <profile>\geode\unzipped\absolllute.hackmega\absolllute.hackmega.dll --gd <runtime511>/GeometryDash.exe --geode <runtime511>/Geode.dll --harness <installed>/megahack.trace.geode --profile runtime511 --gd-version 2.2081 --geode-version 5.11.2 --scenario <manifest> --settings <live hackpro.json> --tag-key 0x1E2=flag:CHEAT_SAFETY/HACK_SAFE_MODE --repetition 0 --context <path>`
   `--target` is the **installed** DLL (what the game loads), not the sample it was packaged
   from - the two can differ, and did (see §Target identity).
3. Launch with `MH_TRACE_CONTEXT=<path>` in the environment, load one level, quit. Without a
   context the trace is still written, with `null` fields and `capture.status: partial`.
4. The trace appears at `%LOCALAPPDATA%\GeometryDash\geode\mods\megahack.trace\traces\<run_id>.jsonl`
   (Geode redirects mod data there, not into the profile folder); grade it with
   `harness/build/mh_trace_check <trace>` (exit 0 means structurally valid, and it prints
   every issue the reader found).
5. `python harness/tools/preflight.py --sidecar <trace>` writes its digest.

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
