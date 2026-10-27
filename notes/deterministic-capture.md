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

1. **Context plumbing** — `preflight.py` copies `timing.max_frames` (and the scenario's `stop_on`
   names) into `pending-context.json`; the observer parses it with the same defensive reading as
   the other context fields, and a missing value means "no bound" with `capture.reasons` naming
   it, never a silent default.
2. **Enforcement** — in the single write path, once `frame > max_frames`, stop emitting events and
   record the reason. `end.counts` must keep distinguishing "stopped at the manifest's bound" from
   "dropped because the writer failed"; a bounded run is `capture: complete` with a named stop
   reason, not `partial`.
3. **Scenario** — `harness/scenarios/no-input-smoke.json`, `input.driver: none`, no actions,
   `timing.max_frames` chosen from the fixture (the first block is ~4.8 s in, so 600 frames at
   60 fps covers a death and one restart), same four required hooks, `0x16c8b0` optional.
4. **Captures** — two per variant, same four-step cycle as before (preflight → install → launch →
   restore/grade), with the operator told to touch nothing.
5. **Acceptance** — within a variant, the two traces have equal event counts for the per-frame
   hooks and no `unstable` row; across variants, no `fail` row; the level observation still equals
   the pin. If a within-variant pair still differs, the difference is diagnosed from the trace
   rather than absorbed into a tolerance.

## What this deliberately does not do

- It does not add a tolerance, a volatility entry, or a "counts may differ" rule. The comparator's
  complaint is correct; the fix belongs in the capture.
- It does not change `safe-mode-smoke` or its recorded evidence.
- It does not reproduce Safe Mode inside the reconstruction; that stays out of scope as recorded
  in [safe-mode-feature.md](safe-mode-feature.md).

## Open questions

- **Where does the frame counter come from** in the observer today, and is it monotonic across
  `PlayLayer` restarts? If it resets per attempt, a `max_frames` bound must be defined against
  the trace, not the attempt.
- **Should the bound be `max_frames` or wall-clock?** Wall-clock is what a human can follow, but
  only frames are comparable across runs, so the bound should be frames and the operator
  instruction derived from it.
- **Does a no-input run reach any target-dependent site at all?** If it does not, no scenario
  shape can make the claim `behavioural` without input injection, and D3 moves from "deferred" to
  "required" — which is worth knowing before building the injector.
