# Trace-harness build loop

This file is the normative instruction for the autonomous build loop. One run = one
iteration. Work in the same session across iterations; never assume memory survived
compaction — re-establish state from files.

AGENTS.md is the contract, not a suggestion.

## STEP 0 — ORIENT (cheap, every iteration)

1. Read `harness/PROGRESS.md`. If it is missing, this is iteration 1: create
   `harness/PROGRESS.md` with the BACKLOG table below (id, item, status) and
   `harness/CMakeLists.txt` (C++20, no Geode, no Windows API), commit both, and report.
   Do not start an item yet.
2. Read `docs/ROADMAP.md` M3 criterion 4 and the sections "Trace format",
   "Comparability gate" and "Comparison rules" of `notes/runtime-validation.md`.
   Do not re-read the notes tree.
3. Take the first backlog item whose status is not done. If none is left, go to STEP 4.

## STEP 1 — DO ONE ITEM

Write the test first, then the implementation, in `harness/` only. The trace contract is
normative: schema strings, field names, value-object states, canonical event keys and
result statuses come from `notes/runtime-validation.md` verbatim. Do not invent fields,
do not rename them, do not add a second dialect. No Geode headers, no Windows API, no
third-party dependencies.

## STEP 2 — VERIFY (a red gate is a stop, not a warning)

- Build and run the harness tests (cmake+ninja or g++); all must pass.
- `python scripts/check_doc_links.py`
- `python scripts/make_progress_svg.py --check`
- if you touched `scripts/` or `notes/`, the matching `scripts/test_*.py`

If a gate fails and the cause is your item, fix it. If the cause is outside the item,
revert the item, mark it blocked with the reason in PROGRESS.md, and stop the loop.

## STEP 3 — RECORD AND COMMIT

Update `harness/PROGRESS.md` (status plus evidence: test names, commit subject).
Commit only the files this item touched, Conventional Commits, e.g.
`feat(harness): value model with bits-exact float equality`. Never sweep unrelated dirty
files. Never touch `samples/`, `recovered/` or `notes/android-input-parity.md`. Push
every fifth iteration and at close.

## STEP 4 — CLOSE

Only when every backlog item is done: run the full verification set, write
`notes/trace-harness.md` (question, method, evidence, conclusion, open questions), add a
short harness section to `README.md`, notify with `scripts/notify.ps1 -Kind Done`, stop
the watchdog if one is armed, and call `stop_loop` with reason "trace core complete".
Do NOT mark any M3 criterion complete: runtime validation still needs a captured trace
from the game.

## HARD RULES

- Evidence or it does not exist. New code cites the contract section it implements; no
  claim about the release without file, address and tool.
- Never mark a roadmap criterion complete, never edit `samples/`, never commit a binary,
  never invent a number.
- If an item needs a decision only the user can make, send exactly one
  `scripts/notify.ps1 -Kind Question`, set the item to blocked, commit, and `stop_loop`.
  Do not guess and do not ask twice.
- If the same gate fails twice for reasons outside the item, `stop_loop` and report.
- One iteration is under ~15 minutes of work. If an item is bigger, split it in
  PROGRESS.md first and do the first half only.

End every iteration with a report under 15 lines: item, what changed, gates run, next
item.

## BACKLOG (in order)

| id | item | status |
| --- | --- | --- |
| 1 | harness skeleton: CMakeLists.txt, tests/ runner, PROGRESS.md | pending |
| 2 | Value model: observed(u64, i32, f32 with bits, bool, str, bytes, ptr), null, unavailable, redacted; equality rules; JSON serialisation | pending |
| 3 | Canonical JSON writer: sorted keys, no insignificant whitespace, UTF-8, plus run_fingerprint as SHA-256 over the canonical object (own SHA-256, no deps) | pending |
| 4 | Trace writer: meta/event/end, seq from 0, attempted/dropped counters, capture status, JSONL, no BOM | pending |
| 5 | Call pairing and call-stack validation: unique call_id, depth/parent chain, unclosed enters, exit without enter, reused id | pending |
| 6 | Trace reader/validator: required keys present, seq gaps, end record required, capture gate | pending |
| 7 | Scenario manifest parser and validator (mh-scenario/1): required fields, status vocabulary, level/profile/settings rules | pending |
| 8 | Comparator core: comparability gate, volatile-field normalisation, canonical event keys | pending |
| 9 | Per-key sequence alignment: min_count on both sides, LCS/Needleman-Wunsch with the documented tie-breaks, missing vs extra classification | pending |
| 10 | Ordering groups and value comparison: main-thread only, tolerance formula with a mandatory "why", result statuses | pending |
| 11 | Synthetic corpus: one pair per status (pass, pass_with_warnings, fail, blocked, unstable, invalid), including the seven-line example trace from notes/runtime-validation.md, which must validate and round-trip | pending |
| 12 | Docs: README harness section and notes/trace-harness.md test matrix | pending |
