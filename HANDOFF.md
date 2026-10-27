# Handoff: continue the Mega Hack runtime-validation work

Paste this file (or point the new session at it) as the opening message. It assumes a fresh
context, no memory of the previous session, and no prior reading of the notes.

State recorded at commit `69c64bb` (2026-10-27), branch `migration/curated-codebase`, in sync
with `origin`. If `git log -1` disagrees, trust the repository and re-read the two notes named
in §3 before acting.

## 1. What this repository is

`C:\Users\Robert_\Documents\megahack-research-new-codebase` — a reverse-engineering and
preservation workspace for **Mega Hack v9.1.3** (the Geode mod `absolllute.hackmega`) running
on **Geometry Dash 2.2081 / Geode 5.11.2**. It holds research notes, original analysis
tooling, a trace harness, and a host-testable reconstruction of the mod's observed surface. It
does not ship, redistribute or improve the mod.

**Read `AGENTS.md` first and treat it as binding.** The rules that get broken most often:

- Ground truth is `samples/absolllute.hackmega/absolllute.hackmega_original.dll`, 8,697,056
  bytes, SHA-256 `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`, image base
  `0x180000000`. **`samples/…/absolllute.hackmega.dll` and `samples/absolllute.hackmega.geode`
  are a different build (`24cebcac…`) — never install them.**
- `samples/` and `recovered/` are evidence: read, never edit, never commit. `notes/android-input-parity.md`
  is deliberately untracked (`??`) — leave it untracked and unmodified.
- Evidence or it does not exist: every address, offset and claim carries file, VA and the tool
  that produced it. Verify twice with two independent angles. Write
  `UNVERIFIED: <what would close it>` rather than promoting a plausible reconstruction.
- Never commit binaries, assets, traces or user data. Never mark a roadmap criterion complete
  without the evidence that criterion names.
- Python is `C:\Users\Robert_\AppData\Local\Python\bin\python3.exe` (`python` on PATH is a
  Store stub). Ghidra/REA MCP is the second analysis engine; a cold import costs ~6 minutes, so
  batch whole-program questions and `close_binary(snapshot_path=recovered/…)`.

## 2. Where the work actually stands

Everything below was verified on 2026-10-27 and is recorded in
`notes/deterministic-capture.md` ("Result") and `harness/PROGRESS.md` (item 17).

| Thing | State |
| --- | --- |
| Static map | 18,825 / 19,496 functions decompiled; 338 hook registrations over 215 GD methods; 125 tag registrations; 186 same-image hooks across 111 callbacks; 28 / 33 exit criteria |
| Trace core (`harness/`) | dependency-free C++20 `mh_trace`; **213 test cases, 7 ctest entries, all green**; build `cmake -S harness -B harness/build -G Ninja && cmake --build harness/build` |
| Observer | Geode mod `megahack.trace` **v0.2.0**, package SHA-256 `27fdc58a…`, enforces a frame-bounded window |
| Reconstruction | Geode mod `megahack.reconstruction` **v0.0.1**, DLL `5f214c5f…`, installs the same six hooks the observer watches and produces **no behaviour** ("hook surface only, no trace") |
| Scenario | `harness/scenarios/no-input-smoke.json`, SHA-256 `5c77e28b…`, `status: draft`, window anchor `level_started`, `max_frames: 600`, safe-mode tag `0x1E2 = true` |
| Captures | four evidence-grade runs (two per variant), all `capture: complete`, 1,210 events, `end.frames = 600`, identical hook census `0x3981d0` 8 / `0x3b4ba0` 1198 / `0x3b8eb0` 2, level `58d4b320…` |
| Comparison | status `pass`, gate `pass`, unstable `no`, `claim: surface` |

Release run ids `874caa47` / `3838cf41`, reconstruction `e5ddd6f2` / `27cffad5`; the four
traces are event-for-event identical once run identity is stripped and frames are rebased.

**`claim: surface` is the correct result, not a shortfall.** `derive_claim` needs a record kind
that only the target can produce, in *both* variants. The only registered target-dependent site
is `0x16c8b0` (`GJGameLevel::savePercentage`), and it is installed in every run yet fires **zero
times in all four** — the fixture dies and restarts and never writes a percentage inside 600
frames. So all five clusters in `notes/m3-validation.md` remain structural-only, and M3's
fourth criterion stays unchecked.

M5 is the other in-progress milestone: ARM64 is mapped (3,052 `_ZN` symbols, no typeinfo), but
on-device behaviour and an NDK/aarch64 build of the portable core are outstanding.

## 3. Read before designing anything

- `notes/deterministic-capture.md` — why frames are rebased (not declared volatile), what the
  four captures cost, and the **open structural question** about the claim class (§ next).
- `notes/runtime-validation.md` — the normative trace contract, scenario manifest, comparability
  gate and comparison rules. Do not invent fields or a second dialect.
- `notes/m3-validation.md` — the five clusters and what would promote each.
- `harness/PROGRESS.md` item 17 + `## Open questions` — the implementation's own unresolved
  questions (tie-break reading, canonical payload, non-finite floats, hex case).
- `notes/address-map.md` — the index of indexes. Search before starting an investigation.

## 4. The next task, and why it is this one

**The open structural question** (`notes/deterministic-capture.md`, last open question):
`derive_claim` requires the target-dependent kind in *both* variants, but the release's whole
purpose at `0x16c8b0` is to *suppress* the write when `0x1E2` is set. The five-run comparison
saw exactly that asymmetry — `save_requested` in both reconstruction runs, in neither release
run. Under the current definition, "the release suppresses what the reconstruction performs"
can never lift the claim above `surface`, which is a gap in the claim class rather than in the
evidence.

Two candidate resolutions, and neither is an edit to make unilaterally:

1. **Widen the claim class** so a kind present in one variant and *provably absent* in the
   other counts as target-dependent. This is a contract/manifest decision that needs its own
   evidence.
2. **Run a scenario with the safe-mode tag off** so both variants write. `UNVERIFIED: whether a
   `0x1E2 = false` scenario would make both variants reach `0x16c8b0` — that is the thing to
   measure, and it needs a run that produces a percentage write at all.

Both resolutions are downstream of the same prerequisite, which is why it is the next task:

> **Make a bounded run reach `0x16c8b0` at all.** The current fixture's one-block layout
> produces a death and a restart and never a percentage write. A new level fixture (generated
> by `harness/tools/make_level_fixture.py`, installed only through the save sandbox) plus a
> scenario that reaches a percentage write — completion, or the game's own periodic save — is
> the lever. Input injection is *not*: jumps do not create progress.

Wire it as: amend or add a manifest (note that the manifest pins `meta.scenario.sha256` and the
harness package digest, so **a manifest amendment invalidates every existing capture** — retake
them, do not retro-fit a declaration), regenerate the level fixture, run `preflight.py` twice
per variant, grade each trace, then compare. Expect the census to change; that is the point.

If a game launch is not available in the session, the parallelisable work is static and safe:
M3 criterion 1 (284 of 389 changelog features are still `cluster`, 106 mapped —
`notes/feature-inventory.md`) and M3 criterion 2 (per-feature VAs, settings keys and defaults).

## 5. Run book (the parts that cost time to learn)

Build and test the core:

```
cmake -S harness -B harness/build -G Ninja
cmake --build harness/build
ctest --test-dir harness/build --output-on-failure     # 7 entries, 213 cases
```

Build a Geode mod (observer or reconstruction; `$env:GEODE_SDK` must be set, e.g.
`C:\Users\Robert_\Documents\Geode`):

```
cd harness\observer        # or reconstruction\geode
geode build                # first build after a configure change exits 1 while packaging; re-run gives 0
```

Package outside the repo (without `-o` it writes into `samples/`):

```
geode package new <dir> -o <somewhere outside the repo>.geode
```

Preflight — one run context per launch, written fresh every time:

```
& $py harness\tools\preflight.py ^
  --target <profile>\geode\unzipped\absolllute.hackmega\absolllute.hackmega.dll ^
  --gd <runtime511>\GeometryDash.exe --geode <runtime511>\Geode.dll ^
  --harness <installed>\megahack.trace.geode ^
  --gd-version 2.2081 --geode-version 5.11.2 --profile runtime511 ^
  --role release --expect-sha256 1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92 ^
  --scenario harness\scenarios\no-input-smoke.json --repetition 0 ^
  --settings %LOCALAPPDATA%\GeometryDash\geode\mods\absolllute.megahack\v9\hackpro.json ^
  --tag-key 0x1E2=flag:CHEAT_SAFETY/HACK_SAFE_MODE --context <ctx>
```

Launch from `C:\Users\Robert_\Documents\GD-Runtime-511` with `geode profile run runtime511`,
**as a background process** — a foreground launch that hits a timeout kills the game. Grade and
compare:

```
& $py harness\tools\grade_trace.py --latest
harness\build\mh_trace_compare.exe harness\scenarios\no-input-smoke.json \
    --release <trace.jsonl> --release <trace.jsonl> \
    --reconstruction <trace.jsonl> --reconstruction <trace.jsonl>
```

Gates before claiming anything:

```
& $py scripts\check_doc_links.py
& $py scripts\make_progress_svg.py --check
& $py scripts\check_inventory_evidence.py samples\absolllute.hackmega\absolllute.hackmega_original.dll
ctest --test-dir harness\build --output-on-failure
& $py harness\tools\test_preflight.py        # and the other harness/tools/test_*.py
```

Anything that may exceed ~60 s runs under `scripts\run_supervised.ps1`; never pipe long output
through `Select-Object -First/Last`, which hides progress.

## 6. Traps, in the order they bite

1. **Never run against the live Steam profile.** Only `runtime511`; `preflight.py` refuses `a`.
   Restore the save from a backup after a run, and verify it (17 level rows, no fixture string).
2. **Run contexts are single use.** The observer consumes `pending-context.json` and renames it
   `.consumed`; a `partial` capture usually means the context was already used.
3. **One target per profile, one target per run.** Never install the release and the
   reconstruction together.
4. **The installed target must be the release.** `--target` names the artifact the game loads
   (the unzipped copy under the profile), not the sample it was packaged from.
5. **`hook.target` volatility.** It is dropped from the comparison payload because the
   registering mod's addresses and tags are variant identity; `hook.key`/`hook.rva` is the
   observed behaviour. Do not "fix" that to make a row pass.
6. **Frames are absolute since launch** (`CCDirector::getTotalFrames()`), so the window is
   anchored at a milestone and the comparator rebases. Rebase, never declare `frame` volatile —
   a hook firing one frame late must still differ.
7. **`comparison.volatile` is applied everywhere** — if you add a field there, thread it through
   `comparison_payload`, `payloads_by_key`, `variant_differences`, `variant_is_unstable`,
   `note_variant_drift` and `align_by_key`, and declare it only after observing it vary.
8. **PowerShell `>` writes UTF-16.** Never redirect git output into a file that becomes a
   manifest; write bytes from Python. `Expand-Archive` rejects `.geode` — copy to `.zip` first.
9. **Never list `%LOCALAPPDATA%\GeometryDash` unfiltered** (hundreds of audio files).
10. **`scripts/notify.ps1` takes `-Text`**, not `-Message`, and the topic is public: status
    words only, no paths, no hashes, nothing from `samples/`.

## 7. How to record a result

`notes/<subsystem>.md` is durable output and needs **question, method, evidence, conclusion,
open questions** — a conclusion without a method is not a note. Tables of addresses beat prose.
Record what failed and why. Correct a contradiction in place (a dated `**Superseded …**` block
keeps history) rather than leaving two versions of the truth. Commit with Conventional Commits,
one coherent change per commit, and check `git status` for `samples/`, `recovered/`, binaries
and scratch files first.

## 8. What is deliberately not done

- `behavioural` has never been claimed, and the pipeline explains why rather than hiding it
  (§4). Do not close M3 by redefining the word without evidence.
- `repetition_index` is written into `meta` although the contract says it never is — harmless
  today (both variants leak the same field), recorded as a contract/implementation mismatch
  worth its own change.
- No input-injection driver exists, and the current scenarios deliberately need none.
- `docs/ROADMAP.md`'s cross-cutting items "automated re-baseline diff on version bump" and
  "decide which `samples/.../tools/` are original and may be tracked" are still open.
