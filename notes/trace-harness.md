# Trace harness: the host-testable trace core

- **Question** — the runtime-validation contract ([runtime-validation.md](runtime-validation.md))
  defines a trace format, a scenario manifest and a comparison procedure, but none of it
  was executable: "No trace has been captured; all five clusters in
  [m3-validation.md](m3-validation.md) remain structural-only." Can the contract be turned
  into code whose behaviour is checkable without a game, a Geode install or a captured run?
- **Method** — `harness/` implements the contract as a dependency-free C++20 library plus a
  test suite, and validates it against the contract's own worked examples rather than
  against invented ones. Where the contract publishes an example (the seven-line trace, the
  `mh-scenario/1` manifest), a generator copies it out of the note byte for byte
  (`harness/tools/gen_example_*_fixture.py`), so the fixture cannot drift from the text it
  claims to represent.
- **Date** — 2026-10-27.

## What exists

| Unit | Header | Implements |
| --- | --- | --- |
| Contract strings | `mh/contract.hpp` | schema strings, record types (§Trace format) |
| Value model | `mh/value.hpp` | §Value objects, §Comparison rules 5 |
| JSON tree | `mh/json.hpp` | canonical and document-order rendering |
| JSON parser | `mh/json_parse.hpp` | strict RFC 8259, no BOM, no duplicate keys |
| SHA-256 | `mh/sha256.hpp` | FIPS 180-4, own implementation |
| Fingerprint | `mh/fingerprint.hpp` | §Common fields `run_fingerprint` |
| Trace writer | `mh/trace_writer.hpp` | §Trace format, §Common fields, §`end` record |
| Call stack | `mh/call_stack.hpp` | §Call fields, §Comparison rules 2 |
| Trace reader | `mh/trace_reader.hpp` | structural validation of a trace file |
| Scenario | `mh/scenario.hpp` | §Scenario manifest and its sub-sections |
| Comparator | `mh/comparator.hpp` | §Comparison rules 1, §Comparability gate, §9 |
| Alignment | `mh/sequence.hpp` | §Comparison rules 3 |
| Values and ordering | `mh/comparison.hpp` | §Comparison rules 4, 5, 6, 9 |
| End to end | `mh/compare.hpp` | the pipeline, including §8 stability |

No Geode headers, no Windows API, no third-party dependency. The writer opens trace files in
binary mode so Windows cannot rewrite LF as CRLF; nothing writes a BOM.

## Evidence

Build and run:

```powershell
cmake -S harness -B harness/build -G Ninja
cmake --build harness/build
ctest --test-dir harness/build --output-on-failure
```

`162 test(s), 0 failed`, `ctest` `100% tests passed out of 1`. Repository gates:
`check_doc_links.py` and `make_progress_svg.py --check` both exit 0.

Two angles, not one, for the parts where a single test could agree with itself:

- **SHA-256** is checked against four published FIPS 180-4 vectors (empty, `abc`, the
  two-block 56-byte message, one million `a`).
- **`run_fingerprint`** is checked both against this tree's own canonical writer and against
  a digest computed independently by Python `hashlib` over
  `json.dumps(..., sort_keys=True, separators=(",", ":"))`.
- **The trace contract** is checked against the note's own seven-line example: it validates
  with no error-severity issue, and every line round-trips byte for byte through
  parse → write.
- **The manifest contract** is checked against the note's own example manifest, which is
  `status: draft` and is correctly not eligible for evidence.

## Test matrix

| Test file | Tests | Contract section | What it pins |
| --- | ---: | --- | --- |
| `test_contract.cpp` | 3 | §Trace format | schema strings and the three record types |
| `test_value.cpp` | 13 | §Value objects, §Comparison rules 5 | tagged shapes, bits-exact float equality, pointer exclusion, unavailable ≠ null |
| `test_json.cpp` | 8 | §Common fields | canonical order, no whitespace, escaping, number spelling |
| `test_sha256.cpp` | 5 | — | FIPS 180-4 vectors |
| `test_fingerprint.cpp` | 5 | §Common fields | the six members hashed, stability, order significance |
| `test_trace_writer.cpp` | 10 | §Trace format, §`end` record | seq from 0, attempted/dropped, LF, no BOM |
| `test_call_stack.cpp` | 14 | §Call fields, §Comparison rules 2 | pairing, depth/parent chain, unclosed, reused id |
| `test_json_parse.cpp` | 9 | §Trace format | strictness (BOM, duplicate keys, trailing commas), order preserved |
| `test_trace_reader.cpp` | 13 | §Comparison rules 2, 7 | required keys, seq gaps, end required, capture bookkeeping, crash/timeout |
| `test_scenario.cpp` | 16 | §Scenario manifest | status vocabulary, level/profile rules, hook keys, tolerance `why` |
| `test_comparator.cpp` | 23 | §Comparison rules 1, §Comparability gate | canonical keys, volatile normalisation, every gate row |
| `test_sequence.cpp` | 14 | §Comparison rules 3 | min_count both sides, LCS alignment, the three tie-breaks |
| `test_comparison.cpp` | 21 | §Comparison rules 4, 5, 6, 9 | tolerances, main-thread ordering, status precedence |
| `test_corpus.cpp` | 8 | all of the above | one pair per status, and the example trace round-trip |

The corpus pairs are `pass`, `pass_with_warnings`, `fail`, `blocked`, `unstable` and
`invalid`, generated through the writer and read back through the reader, so a case exercises
the whole pipeline. It found two real defects while being written: a milestone body that set
its `data` member twice, and an optional hook emitted without its exit, which made the trace
structurally invalid and turned the intended `pass_with_warnings` into `invalid`.

## Conclusion

The contract is executable. Every rule that is stated as a rule — schema strings, value
states, capture bookkeeping, call-stack pairing, gate rows, alignment, tolerances, ordered
groups, result statuses — now has code and at least one test that cites the sentence it
implements, and the two worked examples in the note are the fixtures rather than
reconstructions.

This is **not** runtime validation. No trace has been captured from the game, the Geode
observer half of `megahack.trace` does not exist yet, and M3's fourth criterion still needs a
release-versus-reconstruction comparison on a captured pair. Per the working rules, no
roadmap criterion is marked complete by this work.

**Superseded on 2026-10-27.** The observer now exists (`megahack.trace` 0.2.0, packaged
`27fdc58a…`), four bounded captures have been taken, and the release-versus-reconstruction
comparison has been run: `pass`, no unstable row, `claim: surface`
([deterministic-capture.md](deterministic-capture.md), "Result"). An executable contract was
the precondition for that comparison, not the comparison itself - which is why this note's
own conclusion stands and the M3 criterion is still open.

Two contradictions inside the contract surfaced and are recorded rather than smoothed over
(see [harness/PROGRESS.md](../harness/PROGRESS.md), open questions):

- §Common fields says every schema key is always present, but the seven-line example the note
  calls valid omits `meta.mods` and `end.artifacts`. Absence of those two is a warning.
- §Comparison rules 3 compares "canonical payloads" without defining the payload subset;
  `comparison_payload` drops the per-file envelope (`schema`, `type`, `run_id`,
  `run_fingerprint`, `seq`, `capture`) and normalisation drops `hook.target`, which is
  variant identity rather than behaviour.

## Open questions

- **Observer.** What emits the trace inside the game: a Geode mod (`megahack.trace`) with a
  hook bridge, or the runner observing externally? Unverified against the 5.11.2 runtime.
- **Non-finite floats.** JSON cannot represent NaN/Infinity; the model keeps `bits` and emits
  `"value":null` for them. The contract does not say which spelling it wants.
- **Payload equality and tolerance.** `compare_traces` uses exact canonical payloads; a
  scenario that declares a tolerance needs the value-level path
  (`compare_value_json`) rather than the payload-string path. Wiring tolerances into the
  per-key payload comparison is not done.
- **`repetitions` and unstable.** §8 requires each variant to be compared with itself first;
  the implementation compares runs 1..n against run 0 by payload map. A scenario with more
  than two runs is only checked pairwise against the first.
