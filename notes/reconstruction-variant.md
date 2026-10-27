# Reconstruction variant: the second target, and what a pass proves

- **Question** — the comparison is blocked because there is only one variant. What is the
  smallest honest way to add a second one, and how does a `pass` say how much it proves,
  given that `hook.target` is normalised away as variant identity
  ([trace-observer.md](trace-observer.md) §What still blocks a comparison)?
- **Method** — extract the record shapes into the portable core so no mod builds a record by
  hand; add a second Geode mod that installs the observed hook surface and nothing else; derive
  a claim class from the manifest's *required* observations and the events actually present;
  make a placeholder digest missing evidence rather than a mismatch. Every claim below was
  checked by building, by ctest, or against a captured trace.
- **Date** — 2026-10-27.
- **Plan** — `docs/superpowers/plans/2026-10-27-reconstruction-variant.md`.

## The emitter question (decided, and it constrains everything else)

The handoff described the reconstruction as emitting through the shared core. That cannot be
right for *this* comparison: the gate requires one harness identity on both sides
(`harness/src/comparator.cpp`, the `harness` row compares `harness.sha256` from each variant's
trace against the manifest and against each other). Two emitters means two different
`environment.harness.sha256` values, and the gate fails by construction.

So:

| Role | Mod | Writes a trace? |
| --- | --- | --- |
| observer / harness | `megahack.trace` v0.1.0 | yes, for both variants |
| release target | `absolllute.hackmega` v9.1.3 | no |
| reconstruction target | `megahack.reconstruction` v0.0.1 | **no** |

What the reconstruction changes in a run is exactly one thing: the observer's `hook.target`
names `megahack.reconstruction.dll` instead of `absolllute.hackmega.dll` at each shared site.
Since `hook.target` is volatile (`harness/src/comparator.cpp:191-204`), that difference does not
affect the comparison - which is why the claim is `surface` and why that word is printed.

## Record builders (portable core)

`harness/include/mh/records.hpp` + `harness/src/records.cpp`: `hook_object`, `call_object`,
`hook_event`, `milestone_event`, `save_event`, `error_event`, plus `rva_text`/`va_text` and
`hook_target`. Values in, `json::Json` out: no Geode, no filesystem, no clock.

Why it is in the core and not in a mod: two copies with one drifted key would make a
comparison lie without failing anything, and the mod cannot be tested on the host. The
observer now contains no record-building code at all; `verify_record_bytes.py` fails if
`"kind"` reappears in `harness/observer/src/main.cpp`.

**Byte-identical, two angles:**

1. `harness/tests/test_records.cpp` rebuilds each record from its own parsed fields and
   compares with `harness/tests/fixtures/records_captured.jsonl`, which is the payload
   extracted from run `8a59be9e-30f1-4b79-86ce-4ea462be70a1` (seq 1155 hook enter, seq 1
   milestone), with only `call.this.value` normalised.
2. `python harness/tools/verify_record_bytes.py <trace>` re-derives the same payloads from the
   real capture and compares: `identical: 2 record shape(s)`.

A first attempt at extraction proved the fixture matters: the milestone builder had been
written with a JSON `null` for `data`, and the capture has `{"state":"null"}`. The build passed;
the byte comparison did not.

## Claim class

`mh::ClaimClass { surface, behavioural }` on `ComparisonReport`
(`harness/include/mh/compare.hpp`). Printed by `mh_trace_compare` only when the status is
`pass` or `pass_with_warnings`, with a fixed qualifier for `surface`:
`lifecycle surface only; not evidence that the reconstruction behaves like the release`.

The rule (`required_target_dependent_kinds`, `derive_claim` in `harness/src/compare.cpp`):

- `behavioural` requires **both** that the manifest *requires* a target-dependent observation
  and that both variants actually contain a record of that kind.
- The registry holds one site: `GeometryDash.exe:rva:0x16c8b0` → record kind `save`. Those
  snapshots are read from the filesystem on either side of the target's own veto, so their
  digests are the only thing in a trace that depends on target behaviour.
- Everything else is `surface`. A manifest that merely *observes* the site (the
  `safe-mode-smoke` shape, where `0x16c8b0` is in `hooks.optional`) is `surface` even when the
  `save` records are present - the claim is about what was required, not about what happened to
  appear.

Deliberately not implemented: the second half of the rule the user named, "a required error
code the reconstruction must not emit". `mh-scenario/1` has no member that could express it,
and adding one is a schema change. UNVERIFIED: what the member should be called and whether a
required-versus-forbidden error list is the right shape.

## Placeholder digests, and the row that would have passed comparing nothing

`mh::is_placeholder_digest` = 64 characters, all `0`. A digest row where either side is a
placeholder is now `blocked` with its own finding naming the field, not `invalid`
(`harness/src/comparator.cpp`). §9 reserves `invalid` for evidence that contradicts itself;
a zeroed digest contradicts nothing.

The level row is the reason this had to be explicit: the trace's `level.sha256` comes from the
manifest through the run context, so both sides are the *same zeros* and the equality test
would have passed having compared nothing at all. `comparator_gate_blocks_a_level_row_where_both_sides_are_placeholders`
pins it.

One consequence worth naming: a single run per variant used to report `invalid`, because any
gate issue set that status. Too few runs is missing evidence, so the runs row is now `blocked`
and `comparator_gate_blocks_a_single_run` replaces the old expectation.

## The reconstruction target

`reconstruction/geode/`: own `mod.json` (`megahack.reconstruction`, geode 5.11.2, gd 2.2081,
v0.0.1), own `CMakeLists.txt` on the observer's verified recipe, and one `src/main.cpp` with
five `$modify` classes that forward to the original and nothing else.

| Site | RVA | Why it is there |
| --- | --- | --- |
| `PlayLayer::init` | `0x3A5B00` | the observer's `level_started` source |
| `PlayLayer::resetLevel` | `0x3B8EB0` | required hook `0x3b8eb0` |
| `PlayLayer::postUpdate` | `0x3B4BA0` | required hook `0x3b4ba0` |
| `PlayerObject::pushButton` | `0x397F40` | required hook `0x397f40` |
| `PlayerObject::releaseButton` | `0x3981D0` | required hook `0x3981d0` |
| `GJGameLevel::savePercentage` | `0x16C8B0` | optional hook, installed because the release installs it |

Artifacts from this build (not committed, `.gitignore` has `/reconstruction/geode/build/`):

| Artifact | SHA-256 |
| --- | --- |
| `megahack.reconstruction.dll` | `5f214c5f086c6339300a9adfeec94376f8e7f33a119272fda28d879b6117ac1f` |
| `megahack.reconstruction.geode` | `df5961623d01f4d336f7d1f88fc01e55e188182843153981e8566bf6450a3ba7` |
| `megahack.trace.geode` (observer, after the record refactor) | `f3ab5f6097b6675c49d005efac2efbbb44b2a27c30f5d2adebb7a3d7d1a4f726` |

That last row is a re-pin: touching the observer source during the commit split forced a
rebuild, the package digest moved (`8be7118a…` → `9b4065b2…` → `f3ab5f60…`), and the manifest
had to be re-filled and re-hashed. This is the cost of a non-reproducible build made concrete,
and it is why the pin is per build rather than per source tree.

`geode build` reports exit 1 on the first build after a configure change while still packaging,
and 0 on an unchanged re-run - the same behaviour already recorded for the observer, not a new
defect.

## The build is not byte-reproducible, and the manifest pin says so

AGENTS.md asks that a derived binary be regenerable byte-for-byte. This one is not, measured:
deleting the DLL and relinking twice produced `3074f67c…` and `5f214c5f…` with `/Brepro` set on
both compile and link, and `c5291189…` / `0a09d987…` without it. `/Brepro` is kept because it
removes the one input that is certainly not ours (the clock); it is demonstrably not sufficient.
UNVERIFIED: which part of the SDK's link still varies (PE debug directory, PDB GUID, or an
imported static library's timestamp).

Consequence, recorded rather than hidden: `targets.reconstruction.sha256` and `harness.sha256`
in the manifest are pins for **these** artifacts. A rebuild needs both re-filled and the run
context rebuilt, because the manifest's own digest is part of the context.

## Manifest changes

`harness/scenarios/safe-mode-smoke.json`:

- `targets.reconstruction.sha256` and `harness.sha256` filled from the built artifacts above.
- `level.sha256` **left at zero**: `harness/fixtures/runtime/safe-mode-smoke.gmd` does not
  exist, and the gate now reports that row as `blocked` instead of letting it pass vacuously.
- `baseline.geode` stays `5.6.1` and the description now says why: it is the *static analysis*
  reference (the release was read against 5.6.1 bindings), while `runtime_baseline.geode` is
  `5.11.2`, which is what every trace runs on. Neither is an identity input - the gate compares
  the two traces' `environment.geode.version` with each other - so
  `scenario_accepts_a_baseline_that_differs_from_the_runtime` pins that editing `baseline` does
  not invalidate a manifest.
- New manifest SHA-256: `499f219cb1f1cc79f9c5ccbaf9769b6ab510bb90ad2c957e17f0af5288ef9f6`.

## Evidence

| Check | Result |
| --- | --- |
| `cmake --build harness/build` | clean |
| `ctest --test-dir harness/build --output-on-failure` | 6 entries, 100% passed |
| `mh_trace_tests` | `189 test(s), 0 failed` |
| `verify_record_bytes.py` on run `8a59be9e…` | `identical: 2 record shape(s)` |
| `test_verify_record_bytes.py` | 8 tests, OK |
| `test_preflight.py`, `test_grade_trace.py` | all passed |
| `check_doc_links.py` | all links resolve |
| `make_progress_svg.py --check` | 4 graphics up to date |
| `geode build` (observer, reconstruction) | both link and package |
| `mh_trace_compare safe-mode-smoke --release <two release runs>` | `blocked`, one finding: two runs per variant required; **no claim line** |

## Not done, and why

- **No run of the reconstruction target.** Installing a mod into a profile and launching the
  game are user actions. Everything above is build- and test-verified; the observer's changed
  code path (the shared builders) is proven byte-identical, but "it still traces on a real
  session" stays UNVERIFIED until the next capture.
- **No level fixture.** `fixtures/runtime/safe-mode-smoke.gmd` is absent, so the gate will stop
  at the level row as `blocked` even with two runs per variant. The 2.2081 save container and
  `k4` are now recovered and implemented ([gd-save-format.md](gd-save-format.md)); what is
  still missing is the minimal level-dict key set, a playable object string, and a run proving
  GD loads the generated file. The manifest's hash stays a placeholder until then.
- **No behaviour signal.** The `savePercentage` path never fired in two sessions
  ([trace-observer.md](trace-observer.md) §The save-percentage site); the claim machinery is
  ready for it but nothing claims `behavioural` today.
- **Not changed:** `hook.target` normalisation, the trace schema, the manifest schema,
  `mods.required` (which still names `absolllute.hackmega`, so it is wrong for a reconstruction
  run - the gate does not read it, and per-variant required mods would be a schema change).

## Runbook for the first comparison (needs launches)

State left behind on 2026-10-27, all verified:

- `runtime511` `geode/mods/` holds the release target and the rebuilt observer
  (`megahack.trace.geode`, `f3ab5f60…`); `geode/unzipped/megahack.trace` was deleted so the
  loader must extract the new package rather than keep the old DLL.
- `%LOCALAPPDATA%\...\megahack.trace\pending-context.json` is a **release, repetition 0**
  context built from the current manifest: scenario `f499f219…`, harness `f3ab5f60…`,
  target `1635c339…`, tag `0x1E2 = true`.

1. Launch `geode profile run runtime511`, load one local level, play for a few seconds, quit.
   The context is consumed (renamed `.consumed`) and a trace appears under
   `%LOCALAPPDATA%\GeometryDash\geode\mods\megahack.trace\traces\`.
2. `python harness/tools/grade_trace.py --latest` - expect `capture: complete`, four required
   hook keys, `level_started` before `attempt_started`.
3. Build the same context with `--repetition 1` and repeat the launch. Two release runs are
   needed: the gate refuses a single run per variant.
4. Remove `absolllute.hackmega.geode` from `mods/` (remove, do not disable - two targets make
   `hook.target` ambiguous and the run reports `multiple_targets_loaded`), and install
   `reconstruction/geode/build/megahack.reconstruction.geode`. Delete
   `geode/unzipped/megahack.reconstruction` if it exists and `geode/unzipped/absolllute.hackmega`.
5. Two reconstruction runs, same scenario:
   `preflight.py --role reconstruction --target <runtime511>/geode/unzipped/megahack.reconstruction/megahack.reconstruction.dll --expect-sha256 5f214c5f…`,
   repetitions 0 and 1.
6. Read the observer log for `hook_install_failed` and `multiple_targets_loaded`, and confirm
   `meta.mods` names `megahack.reconstruction`.
7. `harness\build\mh_trace_compare.exe harness\scenarios\safe-mode-smoke.json --release <a> --release <b> --reconstruction <c> --reconstruction <d>`.

What to expect, so a surprise is visible: the gate will stop at the **level** row, `blocked`,
because `level.sha256` is still a placeholder and there is no fixture. If two runs per variant
are in place and everything else matches, the finder will name exactly that row; any other
`blocked` or `invalid` row is a real problem. Because the gate stops, **no `claim:` line is
printed** - which is the intended behaviour, not a missing feature.

## Open questions

- **Is `mods.required` worth fixing?** It lists the release for both variants. Nothing enforces
  it today, which is why this was not a blocker, but a reader would believe it.
- **What shape should "a required error code" take** in `mh-scenario/1`, if the behavioural
  claim is ever to cover an error the reconstruction must not emit?
- **Does the reconstruction need to be a `$modify` at all**, or would a hook-only mod with an
  explicit address be closer to what the release does? `$modify` was chosen because the release
  uses it and because the loader resolves the address - but it also means the two variants
  share a hook *mechanism*, which a difference in mechanism would not survive.

## First four-run comparison (2026-10-27)

Two runs per variant, all `capture: complete`, all against the generated fixture, with the
save backed up and restored around every run:

| Run | Variant | Events | Milestones | Hooks observed |
| --- | --- | --- | --- | --- |
| `3ff34864` | release `1635c339…` | 8,327 | 1 `level_started`, 2 `attempt_started` | 4 keys, owner `absolllute.hackmega.dll` |
| `319ce3f6` | release `1635c339…` | 5,524 | 1 / 3 | 4 keys, same owner |
| `d5f0ecf6` | reconstruction `5f214c5f…` | 5,767 | 1 / 2, plus 2 `save_requested` | 4 keys + `0x16c8b0`, owner `megahack.reconstruction.dll` |
| `9a972e9e` | reconstruction `5f214c5f…` | 5,737 | 1 / 4, plus 4 `save_requested` | same |

`mh_trace_compare` with all four:

```
status: blocked | gate: blocked | unstable: no
  blocked level: level fixture digest is the manifest's placeholder (level.sha256); the evidence is not there yet
```

So the gate accepts everything it can check — two runs per variant, target and harness digests
consistent, scenario identical — and stops on the one row that has never been checkable. That is
the whole result: the pipeline works, and the level row is now the single thing standing between
it and a verdict.

Two observations to carry forward:

- **The reconstruction ran clean on its first ever launch**, on the same hooks as the release.
  Only its `hook.target` differs, which is the intended signal.
- **The reconstruction runs called `GJGameLevel::savePercentage` (`0x16c8b0`) and the release
  runs did not.** Four enter/exit pairs in one, four in the other. The observer hooks that site
  in both variants, so the difference is in the game's behaviour, not in the observer: something
  about those two sessions reached the save path that the release sessions did not. Sample size
  is two per variant, so this is a lead, not a finding.
  UNVERIFIED: whether the difference is the variant or the session — closing it needs the same
  level, same duration, same actions, which the operator controls and the traces already record.
