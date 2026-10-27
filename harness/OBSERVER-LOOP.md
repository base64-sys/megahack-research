# Trace observer build loop

Normative instruction for the autonomous loop that builds the runtime half of
`megahack.trace`. One run = one iteration. Work in the same session across iterations; never
assume memory survived compaction — re-establish state from `harness/OBSERVER-PROGRESS.md`.

AGENTS.md is the contract. Read it before the first iteration; it does not change.

## Objective

Turn the host-testable trace core into a working observer, then capture a first real trace in
the isolated `runtime511` profile. The core is done: `harness/` has 162 passing tests, a JSONL
writer, a reader/validator, the manifest validator, the gate and the comparator
(see [trace-harness.md](../notes/trace-harness.md)).

Runtime validation is **not** complete when this loop ends unless a captured pair produces a
`pass`. Do not mark any roadmap criterion complete.

## STEP 0 — ORIENT (cheap, every iteration)

1. Read `harness/OBSERVER-PROGRESS.md` (status + evidence + open questions). If missing,
   create it from the backlog table in this file and stop: that is iteration 0.
2. Read the backlog row for the first item that is not `done`.
3. Skim only the notes that item names. Do not re-read the notes tree.

## STEP 1 — DO ONE ITEM

Test or check first where a test is possible. Keep every new file inside `harness/`,
`notes/trace-observer.md` or `scripts/` (runner only). Never edit `samples/`, `recovered/` or
`notes/android-input-parity.md`. Never commit a binary, a `.geode`, a trace file or a build
tree.

Portable-core rule: code under `harness/src` and `harness/include` must stay free of Geode
headers and Windows API so it keeps building with g++ and stays testable. The Geode mod lives
in `harness/observer/` and may link the core; the core may never include the mod.

No absolute VAs in code. Hook keys come from `_hook_sites.tsv` / `_host_methods.tsv` rows and
are spelled as the trace contract spells them.

## STEP 2 — VERIFY (a red gate is a stop, not a warning)

- `cmake --build harness/build && ctest --test-dir harness/build --output-on-failure`
  (162 tests must stay green; a new test is added when behaviour is added).
- Build the observer when it was touched, with the exact command recorded in
  `notes/trace-observer.md`.
- `python harness/tools/test_*.py` when the runner or a generator was touched.
- `python scripts/check_doc_links.py`
- `python scripts/make_progress_svg.py --check` (regenerate first if a note was added)
- If the failure is outside the item, revert the item, mark it blocked in
  `OBSERVER-PROGRESS.md` with the reason, commit, and stop the loop.

## STEP 3 — RECORD AND COMMIT

Update `harness/OBSERVER-PROGRESS.md` (status + evidence: commands, test names, commit
subject). Update `notes/trace-observer.md` with what was learned, including what failed.
Commit only the files the item touched, Conventional Commits, e.g.
`feat(observer): emit meta and end records on mod load`. Push every fifth iteration and at
close.

## STEP 4 — CLOSE

When the backlog is exhausted, or when the remaining work needs the user:

- If **done**: run the full verification set, finish `notes/trace-observer.md`, notify
  `scripts/notify.ps1 -Kind Done`, stop the watchdog if armed, `stop_loop` with reason
  "trace observer complete".
- If **blocked on the user** (installing into a profile, launching the game, a decision only
  they can make): send exactly one `scripts/notify.ps1 -Kind Question`, set the item to
  blocked with the reason, commit, and `stop_loop` with reason "blocked: needs user". Do not
  guess, do not ask twice, do not launch a game session on your own.
- If the same gate fails twice for reasons outside the item: `stop_loop` and report.

## HARD RULES

- Evidence or it does not exist. Every claim about the release carries file, address and tool.
- Never mark a roadmap criterion complete. Never claim a `pass` without a captured pair.
- `runtime511` is the only runtime profile this loop may touch, and installing or launching it
  is a user action: ask, do not act. Never modify the default `a` profile.
- One iteration is under ~15 minutes. If an item is bigger, split it in
  `OBSERVER-PROGRESS.md` first and do the first half only.
- Leave the tree clean at the end of every iteration: committed, no stray build output.
- Report under 15 lines at the end of each iteration: item, what changed, gates, next.

## BACKLOG

| id | item | status |
| --- | --- | --- |
| O1 | Discover the Geode 5.11.2 mod build path on this machine: SDK layout, exact command, produced artifacts, host toolchain; record it in notes/trace-observer.md | pending |
| O2 | Observer skeleton in harness/observer/: builds as mod `megahack.trace`, links the portable core, writes one meta + end record to its save directory (`traces/`) on load | done |
| O3 | Hook bridge: enter/exit hook events for the pilot's required keys from _hook_sites.tsv, with call ids, depth and main-thread role proven at PlayLayer::init | pending |
| O4 | Runner: hashes (target/harness/GD/Geode), one-target enforcement, trace sha256 sidecar, runtime511 selection, manifest feeding; python tests | pending |
| O5 | Pilot capture in runtime511 and a real read_trace pass, then a first comparison of a captured release pair (needs the user to launch) | pending |
