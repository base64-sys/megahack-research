# Deterministic capture: making a behavioural claim reachable

**Question.** The comparability gate now passes on four real captures, but the verdict is
`unstable`: two runs of the *same* variant disagree with themselves. What has to change for the
pipeline to support a behavioural claim, and what is the smallest change that gets there?

**Answered below** (see *Result*): `pass`, on two frame-bounded runs per variant, with the claim
class still `surface` — and with a measured reason for that, not an assumed one.

**Method.** Read the manifest's declared-but-unenforced timing and input fields, the observer's
frame source, and the two captures per variant from 2026-10-27
([reconstruction-variant.md](reconstruction-variant.md), "Five-run comparison"). Where the answer
depends on game behaviour, it is quoted from the traces rather than inferred.

## Evidence: what actually differs between two runs of one variant

| Pair | Events | Session | Difference that drives `unstable` |
| --- | --- | --- | --- |
| release `b6fae021` vs `c06b599f` | 4,801 vs 9,013 | 63 s vs 82 s | per-frame hook `0x3b4ba0` count, `0x397f40`/`0x3981d0` depth-1 payloads |
| reconstruction `8cd6075c` vs `418de514` | 6,127 vs 5,623 | 87 s vs 34 s | same, plus `0x16c8b0` count (a known variant difference, already a warning) |

`0x3b4ba0` alone accounts for 5,486–5,904 of the events in a run, and it fires once per frame.
A session that runs 19 s longer therefore produces ~1,100 more of them, and the comparator is
right to call that "these are not the same run". The input actions differ too: the manifest asks
for `press`/`release` at frames 30/60 and a quit at 180, but `input.driver` is `operator` and
nothing injects or schedules anything (`safe-mode-smoke.json`, `input`; `notes/runtime-validation.md`
records the same limitation).

So there are exactly two sources of within-variant disagreement:

1. **Session length** — unbounded, because `timing.max_frames: 240` is a promise no code enforces.
2. **Input timing** — hand-played, so jump frames and death counts differ.

## Decision

Close (1) first, and close (2) by removing input from the scenario rather than by injecting it.

**D1 — the observer enforces `timing.max_frames`.** The frame bound travels in the run context,
the observer stops *writing* once the bound is reached, and the trace's `end` says why it stopped.
Bounded capture makes the per-frame hook counts equal by construction, which is the single
largest contributor to the current instability.

**D2 — the deterministic scenario has no input.** A "no-input" scenario lets the player run into
the fixture's first block and die; the operator only launches and quits. Deaths and restarts are
then produced by the level and the physics, not by a human, so the event sequence is reproducible
without an input injector. This is a *new* scenario, not a change to `safe-mode-smoke`: the
existing one exists to exercise the save-veto path, and rewriting it would erase the evidence
already recorded against it.

**D3 — input injection is deferred, and named as deferred.** Synthetic key injection
(`SendInput`/`PostMessage` at frame boundaries) is the general solution for scenarios that need
jumps, but it adds a privileged component whose own timing jitter has to be characterised before
its traces mean anything. The no-input scenario answers "can two runs of a variant agree?" without
that risk. Injection becomes necessary only when a scenario's behaviour requires specific inputs —
which the current fixture does not.

## Consequence for the claim class

Bounded, input-free captures can make `status` reach `pass`, but they do **not** by themselves
upgrade the claim. `mh::derive_claim` returns `behavioural` only when the manifest *requires* an
observation that the target alone can produce, and the only such site in
`safe-mode-smoke.json` (`0x16c8b0`, `savePercentage`) is listed as **optional** precisely because
the release's Safe Mode veto can keep GD out of that path
([safe-mode-feature.md](safe-mode-feature.md), "What the reconstruction does not reproduce").
A behavioural claim therefore needs a scenario where both variants are *required* to reach a
target-dependent site — a manifest decision with its own evidence, taken after the captures
reproduce. It is not something frame-bounding can grant.

## Plan

Implemented as harness **0.2.0** (observer `megahack.trace.geode`
`27fdc58a…`), with three deviations from the sketch above, each forced by the code:

1. **Context plumbing** — `harness/tools/preflight.py` gained `resolve_window(manifest)`: it
   returns `None` when `timing.window` is absent, refuses an anchor other than `level_started`, a
   non-positive or non-integer `max_frames`, and any non-empty `timing.stop_on`. The window is
   written to the context as `timing` — a **runner directive**, not a meta member: `read_run_context`
   pulls it out and only splices the rest into `body`, so the recorded meta member set does not
   change. A scenario that declares `max_frames` without a window gets a stderr warning.
2. **Enforcement** — `mh::CaptureWindow` (`harness/include/mh/window.hpp`, `src/window.cpp`) is
   consulted in the observer's single write path. It opens on the record whose `opens_window` flag
   is set (`level_started`), admits records up to `max_frames` frames later, and then closes: the
   first record past the bound is dropped and everything after it, including frame-less records.
   On exit the observer writes `end.status = completed` and `end.frames = <last - first + 1>` when
   the window closed, `stopped` when it never did, and logs `capture window did not close` rather
   than guessing a bound. `end.capture` keeps its own, separate meaning (a missing stdout digest),
   so "stopped at the bound" and "the writer dropped records" stay distinguishable.
3. **Scenario** — `harness/scenarios/no-input-smoke.json`: `input.driver` stays `"operator"` with
   `"actions": []`, because the vocabulary has no `"none"` and a human still launches and quits;
   `timing.window = {"anchor": "level_started", "max_frames": 600}` (10 s at 60 fps, past the
   fixture's first block); `hooks.required` is `0x3b8eb0` + `0x3b4ba0` only, with
   `0x397f40`/`0x3981d0`/`0x16c8b0` optional. **Corrected by measurement:** the first draft of
   this item justified the optional input hooks by saying an input-free run never fires
   `pushButton`/`releaseButton`. That is half wrong, and the four captures prove it — with no key
   touched, `0x3981d0` (`releaseButton`) fires **8 times in every run** and `0x397f40`
   (`pushButton`) fires **zero**. So the engine calls `releaseButton` on paths that have nothing
   to do with the player (death, restart), and it is the *`pushButton`* count that is zero, not
   both. Keeping both optional is still right, but the reason is narrower: a stable engine-side
   call is not the input the scenario wants to describe, and requiring it would make the manifest
   assert something about pressing buttons that the scenario deliberately never does.
4. **Captures** — two per variant, same cycle as before (preflight → install → launch →
   restore/grade), operator touching nothing after the level starts.
5. **Acceptance** — unchanged: within a variant, equal per-frame hook counts and no `unstable`
   row; across variants, no `fail` row; the level observation still equals the pin; a residual
   difference is diagnosed from the trace, never absorbed into a tolerance.

## Result: four captures, one verdict (2026-10-27)

Two bounded, input-free runs per variant, all under manifest
`5c77e28b1029eebcd37c928dee6ffc89dc5e3592954d7e808265b93cd13c3f14` and observer
`27fdc58a…` (harness 0.2.0), each launched after a fresh `preflight.py` context and graded
before it was admitted to the comparison:

| rep | variant | run | trace sha256 |
| --- | --- | --- | --- |
| 0 | release `1635c339…` | `874caa47` | `dcdaa095e461c486…` |
| 1 | release `1635c339…` | `3838cf41` | `08a3737bf049e7a3…` |
| 0 | reconstruction `5f214c5f…` | `e5ddd6f2` | `8f697340af6b7362…` |
| 1 | reconstruction `5f214c5f…` | `27cffad5` | `ca51c9888c4c1a8d…` |

Every run: `capture: complete`, `end.status = completed`, `end.frames = 600`, 1,210 events,
0 dropped, 0 unclosed, 604/604 enter/exit, 1,210/1,210 on the main thread, `level_started` =
`58d4b320…`, and the **identical hook census 8 / 1198 / 2** (`0x3981d0` / `0x3b4ba0` /
`0x3b8eb0`). The census matching across *variants* is expected — the observer installs those
hooks in both — but matching across *repetitions* is what D1 promised and this is the first time
it has held on real captures.

```
$ harness\build\mh_trace_compare.exe harness\scenarios\no-input-smoke.json \
    --release <874caa47> --release <3838cf41> \
    --reconstruction <e5ddd6f2> --reconstruction <27cffad5>
status: pass | gate: pass | unstable: no
claim: surface
EXIT=0
```

No `unstable` row, no cross-variant `fail`, no alignment row of any kind. Three things in that
output are the increment, and one thing is *not*:

1. **The frame rebase made within-variant comparison real.** The comparator compared `frame` as
   an absolute number, and the engine's counter is absolute since launch, so two runs of one
   variant could never agree. `comparison_payload()` (`harness/src/compare.cpp`) now rewrites each
   document's `frame` to `frame - level_started_frame + 1` using that document's *own*
   `level_started`; a document without `level_started` keeps absolute frames. The rebase is
   deliberately *not* a volatility declaration: a hook that fires one frame later than its
   counterpart must still show up as a difference, which is why the subtraction rebases rather
   than hides. The payload is rebuilt member-by-member because `json::Json::set` throws
   `json: duplicate object key frame` on an existing key.
2. **The proof is on the older captures, not these.** Re-running the same comparator over the
   previous four 0.2.0-era captures with the *pre-amendment* manifest (extracted byte-exactly
   from `git show HEAD:…`, `c2f5bcdb…`) produces **zero** `unstable release variant` rows where
   the unfixed binary produced six. The release pair is stable on every key purely because of the
   rebase — that is real-trace evidence, independent of the three synthetic tests added with it.
3. **`comparison.volatile` was dead code and is now applied.** `scenario.cpp` parsed it and no
   consumer read it. It is threaded through `comparison_payload`, `payloads_by_key`,
   `variant_differences`, `variant_is_unstable`, `note_variant_drift` and `align_by_key` now, and
   the manifest declares `call.call_id` / `call.parent_call_id` **after** observing them vary —
   the order the contract requires ("record the varying field in `comparison.volatile` only after
   it has been observed to vary"). Because those run-identity fields live in the manifest, the
   four earlier captures became inadmissible against the amended bytes; they were retaken rather
   than retro-fitted, since a declaration written to fit old data is indistinguishable from a
   declaration chosen to make a verdict pass.
4. **The claim class is still `surface`, and that is correct.** `mh::derive_claim`
   (`compare.cpp:312`) returns `behavioural` only when the manifest *requires* a record kind that
   only the target can produce; the single registered site is `0x16c8b0`/`save`
   (`kTargetDependentSites`, `compare.cpp:265`). This scenario requires `0x3b8eb0` and `0x3b4ba0`
   — hooks the observer installs in either variant — so the kind set is empty and `surface`
   follows from the manifest, not from the traces. The pass therefore says the two variants are
   indistinguishable **at this surface**: same hook sequence, same phases, same relative frames,
   same payloads once run identity and the documented volatiles are removed. It says nothing
   about behaviour, and the comparator's own note says so in the output.

**Independent second angle.** A verdict produced by the comparator cannot be its own
corroboration ([AGENTS.md](../AGENTS.md): "one angle is a lead"). A separate script stripped
`t_ns`, `seq`, `run_id`, `run_fingerprint`, `frame`, `call.call_id` and `call.parent_call_id`,
rebased `frame` per document, and compared whole records: all four traces are **event-for-event
identical at 1,210 events**, and the only residual differences are `call.call_id` (declared
volatile in the manifest) and `call.this.value` — a raw heap pointer such as `0x20F3BE3D590` vs
`0x2170E3CA010`, which `mh::normalize_event` removes by design
(`default_volatile_paths()`, `harness/src/comparator.cpp`). Event payloads live under the record
member `data`, not `payload`.

**What the four runs cost to trust.** The observer deletes `pending-context.json` when it consumes
it, so a stale context cannot be replayed: every run needed a fresh preflight, and each context
was read back (repetition, role, target, scenario sha, level pin, window) before launch. The
release package was swapped in only after the reconstruction runs were complete, so exactly one
target was present in every session; the loader log was read for `multiple_targets_loaded` and
`hook_install_failed` and neither appeared. The save was backed up before the first run and
restored after the last, and the restore was verified independently (17 level rows, no fixture
string).



- It does not add a tolerance, a volatility entry, or a "counts may differ" rule. The comparator's
  complaint is correct; the fix belongs in the capture.
- It does not change `safe-mode-smoke` or its recorded evidence: that manifest still pins harness
  `0.1.1`/`4b70fdec…`, and the new observer does not retro-fit it.
- It does not reproduce Safe Mode inside the reconstruction; that stays out of scope as recorded
  in [safe-mode-feature.md](safe-mode-feature.md).

## Open questions

- **Answered: where the frame counter comes from.** `CCDirector::getTotalFrames()`, absolute since
  launch and monotonic across `PlayLayer` restarts, which is why the bound has to be anchored at a
  milestone rather than counted from trace open (`level_started` was at 32460, 37963, 46872 and
  11541 in the four captures of 2026-10-27).
- **Answered: frames, not wall-clock.** The window counts frames; `timeout_ms` stays an operator
  instruction because a wall-clock stop is not comparable across runs.
- **Answered: does a no-input run reach any target-dependent site?** No — and the answer is
  stronger than "we saw none", because the observation is bounded. `0x16c8b0` (`savePercentage`)
  is installed by the observer in every one of the four runs (its hook line appears in each
  launch log) and fired **zero times in all four**; the hook census is exactly
  `0x3981d0` 8, `0x3b4ba0` 1198, `0x3b8eb0` 2, and there are no `save`/`save_requested` records in
  any trace. So the fixture's one-block layout produces a death and a restart (`0x3b8eb0` twice,
  `attempt_started` once) and never a percentage write inside 600 frames. This does **not** make
  D3 "required": input injection would add jumps, not completion, so it is the wrong lever for
  this question. The lever is a level and a scenario shape that reach a percentage write.
- **Still open, and now structural: can `behavioural` ever be reached under Safe Mode?**
  `derive_claim` requires the target-dependent kind in **both** variants' runs, but the release's
  whole purpose at `0x16c8b0` is to *suppress* the write when `0x1E2` is set
  ([safe-mode-feature.md](safe-mode-feature.md)); the five-run comparison observed exactly that
  asymmetry — `save_requested` in both reconstruction runs, in neither release run. Under the
  current definition that divergence can never lift the claim above `surface`, which reads as a
  gap in the claim class rather than in the evidence: "the release suppresses what the
  reconstruction performs" is a behavioural difference, and the pipeline can see it. Whether the
  definition should be widened (a kind present in one variant and provably *absent* in the other
  counts as target-dependent), or whether a behavioural claim needs a scenario with the safe-mode
  tag off, is a manifest/contract decision that needs its own evidence — not an edit made here.
  UNVERIFIED: whether a `0x1E2 = false` scenario would make both variants write, which is what
  the second option depends on.
- **Observed while implementing, deliberately not fixed:** `repetition_index` is written into
  `meta` (`TraceWriter::write_meta` merges `body` verbatim, and the context builder used to copy the
  whole document), even though the contract says it is never part of the meta record. It is
  harmless today — both variants leak the same field — but it is a contract/implementation
  mismatch worth its own change rather than a silent edit here.
