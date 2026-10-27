# Deterministic capture: making a behavioural claim reachable

**Question.** The comparability gate now passes on four real captures, but the verdict is
`unstable`: two runs of the *same* variant disagree with themselves. What has to change for the
pipeline to support a behavioural claim, and what is the smallest change that gets there?

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
   `0x397f40`/`0x3981d0`/`0x16c8b0` optional — an input-free run never fires
   `pushButton`/`releaseButton`, so requiring them would make the scenario demand what it removes.
4. **Captures** — two per variant, same cycle as before (preflight → install → launch →
   restore/grade), operator touching nothing after the level starts.
5. **Acceptance** — unchanged: within a variant, equal per-frame hook counts and no `unstable`
   row; across variants, no `fail` row; the level observation still equals the pin; a residual
   difference is diagnosed from the trace, never absorbed into a tolerance.

## What this deliberately does not do

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
- **Still open: does a no-input run reach any target-dependent site at all?** If it does not, no
  scenario shape can make the claim `behavioural` without input injection, and D3 moves from
  "deferred" to "required". The captures are the measurement that answers this.
- **Observed while implementing, deliberately not fixed:** `repetition_index` is written into
  `meta` (`TraceWriter::write_meta` merges `body` verbatim, and the context builder used to copy the
  whole document), even though the contract says it is never part of the meta record. It is
  harmless today — both variants leak the same field — but it is a contract/implementation
  mismatch worth its own change rather than a silent edit here.
