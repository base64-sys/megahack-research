# Trace-harness build progress

Backlog for the `megahack.trace` trace core. Contract: [runtime-validation.md](../notes/runtime-validation.md)
(§Trace format, §Scenario manifest, §Comparability gate, §Comparison rules). Loop instructions:
[LOOP.md](LOOP.md).

Status vocabulary: `pending`, `in progress`, `done`, `blocked`. Every `done` row carries
evidence (test name, gate output, commit subject).

## Backlog

| id | item | status |
| --- | --- | --- |
| 1 | harness skeleton: CMakeLists.txt, tests/ runner, PROGRESS.md | done |
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

## Evidence log

### Item 1 — skeleton

- Layout: `harness/CMakeLists.txt` (C++20, no Geode, no Windows API, no third-party
  deps), `harness/include/mh/contract.hpp` + `harness/src/contract.cpp` (schema strings
  from the contract verbatim), `harness/tests/CMakeLists.txt`,
  `harness/tests/test_main.cpp` (self-contained runner: `MH_TEST`, `MH_CHECK`,
  `MH_CHECK_EQ`, non-zero exit on failure).
- Why a translation unit exists on day one: CMake refuses a target with no sources
  (`No SOURCES given to target`), so the skeleton ships the contract-constant unit
  instead of an empty library.
- Tests: `schema_strings_match_contract`, `record_type_round_trips`,
  `record_type_rejects_unknown_spelling` — `3 test(s), 0 failed`.
- Build: `cmake -S harness -B harness/build -G Ninja && cmake --build harness/build &&
  ctest --test-dir harness/build --output-on-failure` → `100% tests passed out of 1`.
- Unverified: the observer half of `megahack.trace` (a Geode mod) is out of scope here;
  this tree is the host-testable trace core only.
