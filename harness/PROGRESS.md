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
| 4 | Trace writer: meta/event/end, seq from 0, attempted/dropped counters, capture status, JSONL, no BOM | done |
| 5 | Call pairing and call-stack validation: unique call_id, depth/parent chain, unclosed enters, exit without enter, reused id | done |
| 6a | Strict JSON parser (RFC 8259) for the trace reader: no BOM, no duplicate keys, order preserved | done |
| 6b | Trace reader/validator: required keys present, seq gaps, end record required, capture gate | done |
| 7 | Scenario manifest parser and validator (mh-scenario/1): required fields, status vocabulary, level/profile/settings rules | done |
| 8 | Comparator core: comparability gate, volatile-field normalisation, canonical event keys | done |
| 9 | Per-key sequence alignment: min_count on both sides, LCS/Needleman-Wunsch with the documented tie-breaks, missing vs extra classification | done |
| 10 | Ordering groups and value comparison: main-thread only, tolerance formula with a mandatory "why", result statuses | done |
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

### Item 4 — trace writer

- Files: `include/mh/trace_writer.hpp`, `src/trace_writer.cpp`,
  `tests/test_trace_writer.cpp`; `json::Json` gained a narrow `string_value()` accessor so
  the writer can read `kind`/`phase` without probing the tree.
- Seam: the writer owns identity (`run_id`, `run_fingerprint`), ordering (`seq` assigned
  before the write is attempted), counting and line format; the observer supplies the
  payload. A payload that tries to set an envelope key is rejected with
  `std::invalid_argument` — otherwise the file's own counters and its records could
  disagree. An event without `kind` is likewise rejected.
- Consequences of §Trace format written as code: `seq` is consumed even when the write
  fails, `meta`/`end` are excluded from `attempted`, and `hook_enter`/`hook_exit`/`errors`
  are derived from the payload rather than accepted from the caller. `counts.unclosed` is
  an input, because only call-stack validation (item 5) can see an enter that never exits.
- A lost `meta` does not close the writer: events stay attemptable so
  `end.counts.dropped` can report the loss. `meta_written()` is false in that case;
  the precondition for events is "meta attempted".
- `open_trace_file()` opens binary/truncated so Windows cannot rewrite LF as CRLF. No BOM
  is ever written.
- Tests (10): `trace_writer_meta_is_seq_zero_and_first_line`,
  `trace_writer_event_envelope_is_complete`,
  `trace_writer_seq_increments_per_attempted_event`,
  `trace_writer_counts_written_and_dropped_events`,
  `trace_writer_derives_hook_and_error_counters`,
  `trace_writer_end_carries_counts_and_null_artifacts`,
  `trace_writer_capture_statuses_serialise_without_dangling_lists`,
  `trace_writer_refuses_payload_envelope_keys`,
  `trace_writer_requires_meta_first_and_closes_after_end`,
  `trace_file_is_lf_terminated_utf8_without_bom`.
- Build: `44 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

### Item 5 — call pairing and call-stack validation

- Files: `include/mh/call_stack.hpp`, `src/call_stack.cpp`, `tests/test_call_stack.cpp`.
- Pure function `validate_call_stack(events)` over hook events in file order, one stack per
  thread. `call_id` uniqueness is per run, not per thread ("unique within a run"), so two
  threads cannot both own `c1`.
- Error kinds map to the contract's sentence: `exit_without_enter`, `reused_call_id`,
  `depth_mismatch` (a jump in either direction, since the chain implies exactly one depth),
  `parent_mismatch`, `not_innermost`, `unclosed_enter`.
- An exit of a non-innermost call is an error and the frames above it stay open, so they
  surface as `counts.unclosed` instead of being silently closed. `unclosed` counts open
  frames at end of file and each one carries the enter's `seq`, `thread_id` and `call_id`,
  so `end.counts.unclosed` can be explained rather than merely counted.
- Determinism: stacks are keyed by an ordered map, so the report does not depend on
  hash-table iteration order.
- Tests (14): `call_stack_pairs_nested_calls_per_thread`,
  `call_stack_counts_unclosed_enters_without_closing_them`,
  `call_stack_rejects_exit_without_enter`, `call_stack_rejects_reused_call_id`,
  `call_stack_rejects_reused_id_after_close`, `call_stack_is_unique_across_threads`,
  `call_stack_allows_independent_stacks_per_thread`, `call_stack_rejects_depth_jump`,
  `call_stack_rejects_shallower_depth_than_the_chain`,
  `call_stack_rejects_parent_that_is_not_the_enclosing_call`,
  `call_stack_requires_null_parent_at_depth_one`,
  `call_stack_rejects_exit_of_a_non_innermost_call`,
  `call_stack_rejects_exit_depth_that_does_not_match_the_enter`,
  `call_stack_ignores_an_empty_event_list`.
- Build: `58 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

### Item 6a — strict JSON parser

- Split: item 6 needs a parser before it can validate anything, so the backlog now has
  6a (this) and 6b (reader/validator). Item 6 was one iteration's worth of nothing else.
- Files: `include/mh/json_parse.hpp`, `src/json_parse.cpp`, `tests/test_json_parse.cpp`.
- Strict where the contract is strict: a BOM is a parse error (§Trace format "no BOM"),
  duplicate keys are rejected (the canonical writer would have to drop one silently),
  comments and trailing commas are rejected, and nesting is capped at 128 so a corrupt
  document cannot exhaust the stack.
- Numbers: a plain integer literal becomes `integer` when it fits int64, otherwise `real`
  rather than truncated; a fraction or exponent is always `real`.
- Key order is preserved, so a parsed line can be written back unchanged. That is the
  first half of the §Complete illustrative trace round-trip requirement in item 11.
- Tests (9): `json_parse_accepts_the_contract_value_objects` (all ten §Value objects
  spellings from the note, byte-identical after parse → write),
  `json_parse_rejects_malformed_documents` (14 documents),
  `json_parse_preserves_key_order_for_round_trip`,
  `json_parse_decodes_escapes_and_surrogate_pairs`,
  `json_parse_distinguishes_integer_from_real`, `json_parse_rejects_duplicate_keys`,
  `json_parse_rejects_nesting_beyond_the_limit`,
  `json_parse_reports_the_offset_of_a_syntax_error`,
  `json_parse_round_trips_a_nested_document`.
- Build: `67 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.
- Known limitation: exponent notation (`1e3`) is semantically round-tripped but printed as
  `1000`, because the writer emits the shortest decimal. Integer and plain-decimal
  documents are byte-identical.

### Item 6b — trace reader and structural validator

- Files: `include/mh/trace_reader.hpp`, `src/trace_reader.cpp`,
  `tests/test_trace_reader.cpp`, `tests/fixtures/example_trace.hpp` (generated by
  `harness/tools/gen_example_trace_fixture.py` from the note's §Complete illustrative
  trace, so the fixture is byte-exact and regenerable instead of retyped), and
  `json::Json` gained `integer_value()`/`real_value()`/`boolean_value()`.
- The positive fixture is the contract's own seven-line example, because the note declares
  it valid. It is accepted with **no** error-severity issue, which cross-checks the reader,
  the writer and the call-stack validator against the same document.
- Checks: per-record required keys and schema string, `type` vocabulary, meta first,
  end last and present, duplicate end, seq continuity from 0, run_id/run_fingerprint
  stability, capture.status vocabulary, `capture.missing` ↔ `capture.reasons` coverage,
  thread.id/role, hook events carrying phase/hook/call, `end.status` vocabulary,
  counts.events vs parsed events, crash/timeout as `run_failed`, dropped events as a
  warning, and the call stack itself feeding `call_stack_error` and `unclosed_mismatch`.
- Severity decisions, both forced by the contract's own example: `capture.status=
  unavailable` is a **warning** (the example's save observation is unavailable and the
  note calls the file valid; "the comparability gate, not the record alone, decides whether
  a partial field is required"), while `end.status=crashed|timeout` is an **error**
  (§Comparison rules 7: "A crash in either variant is a failure").
- Contradiction found and recorded: §Common fields says "Every key defined by this schema
  is present even when its value is null. An absent key is invalid", but the seven-line
  example omits `meta.mods` and `end.artifacts`. The example is declared valid, so those
  two are warnings; everything the example carries is an error when absent.
- Tests (13): `trace_reader_accepts_the_contract_example_trace`,
  `trace_reader_requires_an_end_record`, `trace_reader_rejects_a_seq_gap`,
  `trace_reader_rejects_a_parse_error_and_names_the_line`,
  `trace_reader_rejects_the_wrong_schema_string`,
  `trace_reader_rejects_run_identity_changing_mid_file`,
  `trace_reader_rejects_capture_without_a_reason`,
  `trace_reader_rejects_a_record_after_end`, `trace_reader_detects_an_unclosed_call`,
  `trace_reader_rejects_counts_that_disagree_with_the_events`,
  `trace_reader_reports_dropped_events_as_a_warning`,
  `trace_reader_rejects_a_crashed_run`,
  `trace_reader_rejects_non_object_and_empty_lines`.
- Build: `80 test(s), 0 failed`, `ctest` `100% tests passed out of 1`. The fixture
  generator was run twice with an identical result.

### Item 7 — scenario manifest (`mh-scenario/1`)

- Files: `include/mh/scenario.hpp`, `src/scenario.cpp`, `tests/test_scenario.cpp`,
  `tests/fixtures/example_scenario.hpp` (generated by
  `harness/tools/gen_example_scenario_fixture.py` from the note's own manifest).
- `read_scenario` returns a typed `Scenario` plus issues, and the manifest's SHA-256 of the
  exact bytes it was handed, because "The trace's `scenario.sha256` is the SHA-256 of the
  manifest file bytes". Whitespace is part of that identity, which the tests pin.
- Enforced, each citing the sentence it implements: schema string; `status` vocabulary and
  `approved_for_evidence()`; `level.source ∈ {generated, local-fixture}` ("user-level" is
  the observed failure mode); `profile.isolation == disposable`; hook key spelled
  `<module>:rva:<lowercase 0x hex>` or `<module>:name:<method>`; `phases ⊆ {enter, exit}`;
  `min_count >= 1`; milestone vocabulary; tolerance needs a `why` and a JSON-Pointer key;
  settings tags upper-case hex with a bool/int value; `mods.required` lists the observer;
  `quit` takes a null key; `repetitions >= 1` with a warning below the gate's two runs.
- Deliberate non-check: `baseline`/`runtime_baseline` are required to exist but their
  contents are the comparability gate's business (item 8), not the manifest's.
- Tests (15): `scenario_accepts_the_contract_example_manifest`,
  `scenario_sha256_is_the_hash_of_the_file_bytes`, `scenario_rejects_a_wrong_schema_string`,
  `scenario_rejects_an_unknown_status`,
  `scenario_approved_status_is_required_for_evidence`, `scenario_rejects_a_user_level`,
  `scenario_rejects_a_live_profile`, `scenario_rejects_a_short_level_hash`,
  `scenario_requires_the_observer_in_mods`, `scenario_rejects_a_tolerance_without_a_why`,
  `scenario_rejects_an_unknown_milestone`,
  `scenario_rejects_a_hook_key_that_is_not_contract_shaped`,
  `scenario_rejects_a_settings_tag_that_is_not_hex`, `scenario_rejects_quit_with_a_key`,
  `scenario_warns_when_a_single_run_is_requested`, `scenario_rejects_malformed_json`.
- Build: `96 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

### Item 8 — comparator core

- Files: `include/mh/comparator.hpp`, `src/comparator.cpp`, `tests/test_comparator.cpp`.
- `canonical_event_key` implements the §Comparison rules 1 table exactly
  (`observer|key|phase|depth`, `milestone|<name>`, `save|<store>|phase|source`,
  `error|<code>`) and returns `nullopt` when the payload cannot produce a key, because
  "Do not use a runtime VA, timestamp, or thread id in a key" and an incomplete payload has
  no identity to group on.
- `normalize_event` removes §1's list (`t_ns`, `hook.va`, `thread.id`, `call.this.value`,
  `call.duration_ns`, `save.local_path`) plus the variant's own callback addresses
  (`hook.target`). Two implementation notes worth the ink: a parent object left empty is
  dropped, and `save.local_path` is a *semantic* path (the event's `kind` is `save`), not a
  container path — treating it literally silently removed nothing, which the tests caught.
- `check_comparability` walks the gate table row by row and returns `pass`, `invalid`
  (identity: scenario, target, harness, runtime, level, settings, runs) or `blocked`
  (completeness: dropped events, unclosed calls, uncaptured required fields), matching §9:
  "invalid | Comparability gate failed (hash, version, scenario, observer, level,
  settings)" and "blocked | A required field or hook cannot be captured". Identity is
  checked before completeness so `invalid` dominates.
- `summarize_variant` reads the gate's fields out of a variant's traces; a run that is not
  structurally valid counts as crashed, since "each with an `end` record and no
  crash/timeout" is a precondition rather than a detail.
- Tests (18): `comparator_canonical_key_for_a_hook_is_observer_key_phase_depth`,
  `comparator_canonical_key_for_a_milestone_is_the_name`,
  `comparator_canonical_key_for_a_save_is_store_phase_source`,
  `comparator_canonical_key_for_an_error_is_the_code`,
  `comparator_canonical_key_is_null_when_the_payload_is_incomplete`,
  `comparator_normalisation_removes_the_documented_volatile_fields`,
  `comparator_normalisation_removes_call_duration_and_save_local_path`,
  `comparator_normalisation_applies_the_manifest_volatile_list`,
  `comparator_normalisation_ignores_paths_that_do_not_exist`,
  `comparator_gate_passes_when_every_row_matches`,
  `comparator_gate_invalidates_a_different_scenario`,
  `comparator_gate_invalidates_a_different_harness`,
  `comparator_gate_invalidates_a_different_runtime`,
  `comparator_gate_invalidates_a_different_level`,
  `comparator_gate_invalidates_different_settings_tags`,
  `comparator_gate_accepts_reordered_settings_tags`,
  `comparator_gate_invalidates_a_single_run`,
  `comparator_gate_invalidates_a_crashed_run`,
  `comparator_gate_blocks_on_dropped_events`,
  `comparator_gate_blocks_on_an_unclosed_call`,
  `comparator_gate_blocks_on_an_uncaptured_required_field`,
  `comparator_gate_summarises_a_variant_from_traces`,
  `comparator_gate_summary_counts_an_incomplete_trace_as_crashed`.
- Build: `119 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

### Item 9 — per-key sequence alignment

- Files: `include/mh/sequence.hpp`, `src/sequence.cpp`, `tests/test_sequence.cpp`.
- `align_sequences` builds a suffix LCS table and walks forward, which lets the tie-breaks
  be the contract's words in order: exact payload match first, then the earlier release
  event, then the earlier reconstruction event. Adjacent missing/extra steps are coalesced
  into one `changed` pair, because "A gap is a missing event; a mismatched pair is a changed
  event" — a same-length difference must not read as two absences.
- Equality is a caller-supplied `PayloadEqual` predicate, so item 10's tolerance-aware value
  comparison plugs in without this unit re-implementing value rules. A string overload
  keeps the tests readable.
- `check_min_counts` implements the first sentence of §3 on both sides; a required key with
  zero occurrences on either side is a finding, not a vacuous match.
- `align_by_key` reports the union of keys in sorted order, so a key present on only one
  side is reported instead of being silently skipped.
- Terminology in code: `missing` = present in the release only, `extra` = present in the
  reconstruction only, matching the §6 wording "Missing in release or reconstruction".
- Tests (14): `alignment_of_identical_sequences_is_all_matched`,
  `alignment_reports_a_deleted_event_as_missing`,
  `alignment_reports_an_inserted_event_as_extra`,
  `alignment_reports_a_substitution_as_changed`,
  `alignment_prefers_a_match_over_a_gap_that_scores_the_same`,
  `alignment_prefers_the_earlier_release_event_on_a_tie`,
  `alignment_keeps_indices_strictly_increasing`,
  `alignment_of_duplicate_payloads_matches_one_and_reports_the_rest`,
  `alignment_of_an_empty_side_reports_everything`, `alignment_is_deterministic`,
  `min_count_is_enforced_on_both_sides`,
  `min_count_accepts_a_key_absent_from_both_count_maps_only_when_not_required`,
  `align_by_key_covers_keys_present_on_only_one_side`,
  `align_by_key_gives_an_alignment_for_a_key_only_on_the_reconstruction_side`.
- Build: `133 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

### Item 10 — ordering, values and result statuses

- Files: `include/mh/comparison.hpp`, `src/comparison.cpp`, `tests/test_comparison.cpp`.
- `value_from_json` turns a trace value object back into the item-2 `Value`, so the model's
  equality rules (bits-exact floats, pointer excluded, null only null) are applied rather
  than re-invented; a malformed value object is `blocked`, because the trace cannot
  establish either side of the claim.
- `compare_value_json` applies a tolerance only to floats and only when the `why` is
  non-empty — "A tolerance without a `why` is invalid" — and it compares the *decoded*
  values with `abs(a-b) <= abs_tol + rel_tol * max(abs(a), abs(b))`, not the bits. Counts,
  booleans and hashes never take a tolerance here.
- `check_order_group` filters to `thread.role == "main"` inside the checker, because §4
  makes eligibility a property of the evidence ("its events are not eligible to satisfy an
  ordered group"), not of the caller.
- `finalize_status` orders the outcomes the way §9 defines them: `invalid`/`blocked` from
  the gate dominate, `unstable` stops the claim, a `fail` finding beats a warning, warnings
  alone give `pass_with_warnings`.
- Tests (21): the ten §Value spellings through `value_from_json` → `to_json`,
  malformed objects, exact-by-default (including `+0.0` vs `-0.0`), tolerance within and
  outside, tolerance without a `why`, tolerance not applied to counts, unavailable/redacted
  blocked, pointers excluded, null only equal to null; ordering in order, out of order,
  hook-enter keys, non-main threads ineligible, a missing name, an empty group; statuses
  pass, warnings only, fail-beats-warning, invalid dominance, blocked, unstable.
- Build: `154 test(s), 0 failed`, `ctest` `100% tests passed out of 1`.

## Open questions

- **Tie-break reading.** "preferring an exact payload match, then the earlier release
  event, then the earlier reconstruction event" is implemented as: match when it preserves
  the LCS length, else consume the release event first on a tie. UNVERIFIED: whether
  "earlier" means earlier in file order (implemented) or earlier in the alignment; the two
  readings coincide for the cases in this repo's tests but could differ on a long
  insert/delete run.
- **What is a "canonical payload"?** §Comparison rules 3 compares "ordered payload
  sequences" without defining the payload subset. `normalize_event` implements §1's list
  and additionally drops the whole `hook.target` block (the registering mod's addresses and
  tag are variant identity; the observed GD method in `hook.key`/`hook.rva` is the
  behaviour). UNVERIFIED: whether the release-vs-reconstruction comparison should compare
  `hook.target.tag`/`tag_label`; if it should, then a cross-variant payload will also need
  the callback RVA excluded, and this decision moves into item 10.
- **Settings tag order.** The gate compares `settings.tags` as a list sorted by tag, so two
  files listing the same tags in a different order are the same settings. §Comparability
  gate says "same semantic `settings.tags` list" while item 3's fingerprint treats order as
  significant; that difference is deliberate (identity vs semantics) but is a judgement, not
  a quotation.
- **Pattern of `mh-scenario/1` facts.** The example manifest is `draft`, so the validator
  proves shape, not usefulness: nothing in this tree can yet tell whether a scenario's
  `hooks.required` matches a real cluster. UNVERIFIED until a captured trace exists.
- **Required keys vs the contract's example.** "Every key defined by this schema is
  present" cannot be enforced as written: the example the note calls valid omits
  `meta.mods` and `end.artifacts`. UNVERIFIED: whether the example is abbreviated or the
  sentence is aspirational. Until a captured trace exists, absence of those two is a
  warning.
- **Non-finite floats.**

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
