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
| 2 | Value model: observed(u64, i32, f32 with bits, bool, str, bytes, ptr), null, unavailable, redacted; equality rules; JSON serialisation | done |
| 3 | Canonical JSON writer: sorted keys, no insignificant whitespace, UTF-8, plus run_fingerprint as SHA-256 over the canonical object (own SHA-256, no deps) | done |
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

### Item 2 — value model

- Files: `harness/include/mh/value.hpp`, `harness/src/value.cpp`,
  `harness/tests/test_value.cpp`. Test infrastructure moved to `tests/test_runner.hpp`
  and the item-1 tests to `tests/test_contract.cpp` so each item owns its file.
- Contract citations in code: §Value objects for the object shapes, §Comparison rules 5
  for the equality table. `Equality::excluded` exists so a pointer is not silently
  "different" (an excluded field cannot make a run fail, §Comparison rules 5).
- Decisions worth naming:
  - No default constructor and no untyped constructor: a `Value` is always in one of the
    four states, and an observed one always carries a type. A `u64` and an `i32` with the
    same number are different observations, not comparable ones.
  - f32 equality reads `bits` only. `+0.0` vs `-0.0` and two NaN payloads are therefore
    decidable, which is what "Float | Exact `bits`" requires.
  - `unavailable` is never equal to another `unavailable`, per "never equal to anything".
  - Hex is upper case in value objects, matching the contract's own examples
    (`0x3F000000`, `0x1F4A2B00000`); equality is numeric, so this is presentation only.
- Tests (13 in `test_value.cpp`): `value_u64_serialises_as_hex_string`,
  `value_i32_serialises_as_json_number`, `value_f32_serialises_value_and_bits`,
  `value_bool_str_bytes_ptr_serialise_like_the_contract`,
  `value_null_unavailable_redacted_serialise_like_the_contract`,
  `value_strings_escape_json_and_keep_utf8`,
  `value_hex_digits_are_upper_case_and_minimal`,
  `value_observed_equality_is_typed_and_exact`,
  `value_float_equality_uses_bits_not_value`, `value_null_equals_only_null`,
  `value_unavailable_and_redacted_are_never_equal`,
  `value_pointer_equality_is_excluded_by_default`, `value_exposes_state_and_type`.
- Build: `16 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

### Item 3 — canonical JSON and run_fingerprint

- Files: `include/mh/json.hpp` + `src/json.cpp` (tree with two renderings),
  `include/mh/sha256.hpp` + `src/sha256.cpp` (FIPS 180-4, own implementation),
  `include/mh/fingerprint.hpp` + `src/fingerprint.cpp`, `include/mh/text.hpp` (the single
  escaping/hex implementation, now also used by the value model), tests
  `test_json.cpp`, `test_sha256.cpp`, `test_fingerprint.cpp`.
- Two renderings, deliberately: `to_json()` keeps insertion order, so a parsed document
  can be written back unchanged (item 11 needs that round-trip); `to_canonical()` sorts
  keys by UTF-8 byte order, which is what the contract hashes. Hashing the document-order
  form would make the fingerprint depend on key insertion order.
- `Json::real` throws on NaN/Infinity instead of substituting a number: a silently
  different value inside a fingerprint input is the one error a fingerprint cannot detect.
  Duplicate object keys are rejected for the same reason.
- `fingerprint_payload()` is public so the object that gets hashed is visible. Members
  are the six the contract names, spelled as it spells them
  (`scenario.sha256`, `target.sha256`, `harness.sha256`, `settings.tags`,
  `level.sha256`, `repetition_index`); `settings.tags` is the semantic list, not the
  settings file hash, per §meta record "The semantic `settings.tags` list is the
  cross-variant identity."
- Verification, two angles: SHA-256 against four published vectors (empty, `abc`,
  two-block 56-byte message, one million `a`); and the fingerprint digest is pinned to
  `c8439d35…` computed independently by Python `hashlib` over
  `json.dumps(..., sort_keys=True, separators=(",", ":"))` of the same object, not by this
  code.
- Tests (18 new): `json_canonical_sorts_object_keys`,
  `json_canonical_has_no_insignificant_whitespace`,
  `json_strings_escape_and_utf8_passes_through`,
  `json_numbers_are_shortest_round_trip`, `json_empty_containers`,
  `json_object_rejects_duplicate_keys`,
  `json_real_rejects_values_json_cannot_represent`,
  `json_deep_nesting_keeps_order_inside_arrays`, `sha256_empty_message`, `sha256_abc`,
  `sha256_two_block_message`, `sha256_one_million_a`,
  `sha256_digest_text_is_lower_case_64_hex`,
  `fingerprint_payload_is_the_contract_shaped_object`,
  `fingerprint_is_sha256_of_the_canonical_payload`,
  `fingerprint_is_stable_for_identical_input`,
  `fingerprint_changes_when_any_member_changes`, `fingerprint_tag_order_matters`.
- Build: `34 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

## Open questions

- **Non-finite floats.** The contract says floats always carry `bits` and that `value` is
  convenience, but JSON has no `NaN`/`Infinity` literal. The model emits `"value":null`
  for non-finite bits and keeps `bits` exact. UNVERIFIED: whether the writer should
  instead refuse the event, or the contract should name a spelling. Nothing in the
  contract decides it, so a reader must use `bits` for these values.
- **Fingerprint member names.** The contract names the six members as dotted paths
  (`scenario.sha256`), which is the only spelling it gives, so they are used literally as
  object keys. UNVERIFIED: whether a captured trace's writer already uses a different
  spelling; if it does, one of the two must change, and the fingerprint changes with it.
- **Hex case in value objects.** The examples are upper case for `bits`/`ptr` and
  letter-free for `u64`, so the case of `u64` digits is not pinned. Chose upper case
  everywhere for consistency; it cannot affect equality.
