# Reconstruction Variant Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the comparison a real second variant, and make a `pass` say exactly how much it proves.

**Architecture:** The observer stays the single emitter (the gate requires one `harness.sha256`
on both sides, `comparator.cpp:332-340`); `megahack.reconstruction` becomes the second *target*
by installing the same hook surface. Record *shapes* move into the portable core as pure
builders, so a record key cannot drift between mods and is covered by ctest. The claim class is
derived from the manifest's required observations and the events actually observed, lives on the
comparison report, and is printed only when a pass exists.

**Tech Stack:** C++20 (portable core, no Geode), CMake + ctest, MSVC 14.44 via the Geode SDK,
Python 3.14 for the runner tools.

**Spec:** The approved description in this session (user message of 2026-10-27) plus
`notes/trace-observer.md` §What still blocks a comparison, §`hook.target`, and
`notes/trace-harness.md`. Repository rules: `AGENTS.md`.

## Global Constraints

- `harness/include/` stays Geode-free and portable: no Geode headers, no Windows API, no
  filesystem, no third-party dependencies.
- No new schema: the claim is a field on the comparison report, never on the trace or the
  manifest.
- `hook.target` stays in `default_volatile_paths()`; comparator normalisation is not changed.
- Placeholder digests (all zeros) are **blocked**, not `invalid`. A gate row satisfied only
  because both sides are placeholders is not evidence.
- `samples/` and `recovered/` are read-only. No binaries, traces or user data are committed.
- Python is `C:\Users\Robert_\AppData\Local\Python\bin\python3.exe`; `python` is a Store stub.
- One target per profile, one target per run. Only the `runtime511` profile.
- Gates before any commit: `python scripts/check_doc_links.py`,
  `python scripts/make_progress_svg.py --check`, `python harness/tools/test_preflight.py`,
  `python harness/tools/test_grade_trace.py`, `ctest --test-dir harness/build --output-on-failure`.

## Review Focus

- A record builder that emits a different key order or spelling than the observer does today
  would invalidate every trace without failing anything — pinned by byte comparison against a
  captured line (Task 1).
- A claim of `behavioural` derived from a manifest that merely *names* a target-dependent hook
  when no such record was observed (Task 2).
- A reconstruction that quietly fails to install a hook: the run still traces, `hook.target`
  is null, and the comparison passes on the observer's own evidence. The observer's
  `multiple_targets_loaded` / `hook_install_failed` checks and a per-run log read are the guard
  (Task 6).
- A manifest whose declared hashes are refreshed by hand from the wrong artifact (Task 5).
- Two mods at once in one profile: the other target must be removed from `mods/`, not disabled,
  or the observer reports `multiple_targets_loaded` and `hook.target` goes null (Task 6).

---

## File Structure

| Path | Responsibility |
| --- | --- |
| `harness/include/mh/records.hpp`, `harness/src/records.cpp` | pure record builders: `hook`, `call`, `milestone`, `save`, `error` objects → `json::Json` |
| `harness/tests/test_records.cpp` | the shapes, byte-for-byte, plus the fixture from a captured line |
| `harness/include/mh/compare.hpp`, `src/compare.cpp` | `ClaimClass`, `derive_claim`, `ComparisonReport::claim` |
| `harness/include/mh/comparator.hpp`, `src/comparator.cpp` | `GateIssue` severity; placeholder digests → `blocked` |
| `harness/tests/test_compare.cpp` (new), `test_comparator.cpp` (extend) | claim derivation, placeholder mapping |
| `harness/tools/trace_compare.cpp` | prints `claim:` only on pass / pass_with_warnings |
| `harness/observer/src/main.cpp` | calls the builders; recognises both target mod ids in `mods` |
| `reconstruction/geode/` | `mod.json`, `CMakeLists.txt`, `src/main.cpp`: the second target |
| `harness/scenarios/safe-mode-smoke.json` | real `harness.sha256` and `targets.reconstruction.sha256`; baseline declared |
| `harness/tools/verify_record_bytes.py` | re-runs the byte comparison against a real capture |
| `notes/reconstruction-variant.md` | question, method, evidence, conclusion, open questions |

---

### Task 1: Extract the record builders into the portable core

**Files:**
- Create: `harness/include/mh/records.hpp`, `harness/src/records.cpp`, `harness/tests/test_records.cpp`
- Modify: `harness/CMakeLists.txt:18-34` (add `src/records.cpp`), `harness/tests/CMakeLists.txt`
- Modify: `harness/observer/src/main.cpp:354-372, 402-458, 465-503, 513-532`
- Test: `harness/tests/test_records.cpp`

**Interfaces:**
- Produces:
  - `namespace mh::records`
  - `struct HookIdentity { std::string key, method, module; std::uint32_t rva; std::uintptr_t va; }`
  - `json::Json hook_object(const HookIdentity&, json::Json target)`
  - `json::Json call_object(std::string_view call_id, std::int64_t depth, std::string_view parent, bool has_parent, std::uint64_t self, std::optional<std::int64_t> duration_ns)`
  - `json::Json hook_event(std::string_view phase, const HookIdentity&, json::Json target, json::Json call)`
  - `json::Json milestone_event(std::string_view name)`
  - `json::Json save_event(std::string_view phase, bool exists, std::int64_t size, std::string sha256, std::string local_path)`
  - `json::Json error_event(std::string_view code, std::string_view message, bool fatal, std::string hook_key)` (`hook_key` empty → null)
- Consumes: `mh/json.hpp`, `mh/value.hpp`.

- [ ] **Step 1: Capture a baseline line from a real trace**

Read any `megahack.trace` capture under
`%LOCALAPPDATA%\GeometryDash\geode\mods\megahack.trace\traces\`, take one `hook` enter line
for `0x397f40` and one `milestone` line, and note their exact text. These are the bytes the
refactor must reproduce. Copy them into `harness/tests/fixtures/records_captured.jsonl` with
`call.this.value` replaced by `"0x0"` and nothing else changed.

- [ ] **Step 2: Write the failing tests**

`harness/tests/test_records.cpp`, one test per builder, asserting `to_json()` against a literal
string in the observer's current key order (`hook`: `observer, key, module, method, resolution,
rva, va, target, convention`; `call`: `call_id, depth, parent_call_id, this, args, return[, duration_ns]`;
`milestone`: `kind, name, data`; `save`: `kind, store, phase, source, path_template, exists, size,
sha256, keys, local_path`; `error`: `kind, code, message, fatal, hook_key`). Names:
`records_hook_object_matches_the_observer_bytes`, `records_call_object_nests_and_carries_duration`,
`records_hook_event_wraps_phase_and_call`, `records_milestone_event_has_no_invented_data`,
`records_save_event_states_absence_with_nulls`, `records_error_event_nulls_an_absent_hook_key`,
`records_captured_line_round_trips_through_the_builders` (parses
`fixtures/records_captured.jsonl`, rebuilds each record from its parsed fields, asserts equality
with the committed line).

- [ ] **Step 3: Run to verify failure**

Run: `cmake --build harness/build` — expected: `test_records.cpp` fails to compile, `records.hpp`
not found.

- [ ] **Step 4: Implement the builders**

Move the bodies from the observer verbatim: `build_hook_object` → `records::hook_object`,
`emit_hook_locked`'s `call` block → `records::call_object`, the milestone body →
`records::milestone_event`, `save_snapshot_object` (without the filesystem read) →
`records::save_event`, `emit_error_locked`'s body → `records::error_event`. Keep `"0x%X"` for
`rva` and `"0x%llX"` for `va`.

- [ ] **Step 5: Run the tests**

Run: `cmake --build harness/build && ctest --test-dir harness/build --output-on-failure`
Expected: all entries pass, case count 170 + 7.

- [ ] **Step 6: Move the observer onto the builders**

`harness/observer/src/main.cpp` keeps `HookSpec`, the session, the stack, the writer and the
directories; it deletes the five builder bodies and calls `mh::records::*`. `save_snapshot_object`
keeps the file read and passes values in.

- [ ] **Step 7: Build the observer and prove byte identity**

Run: `cd harness\observer; geode build` (twice if the first exits 1 during packaging).
Then run `python harness\tools\verify_record_bytes.py <trace.jsonl>` against the newest capture
and confirm it reports `identical` for every record it rebuilds.

- [ ] **Step 8: Commit**

`git commit -m "refactor(harness): share pure record builders between the core and the observer"`

---

### Task 2: The claim class on the comparison report

**Files:**
- Modify: `harness/include/mh/compare.hpp`, `harness/src/compare.cpp`, `harness/tools/trace_compare.cpp`
- Create: `harness/tests/test_compare.cpp`; modify `harness/tests/CMakeLists.txt`

**Interfaces:**
- Produces: `enum class ClaimClass { surface, behavioural }`,
  `std::string_view to_string(ClaimClass)`,
  `std::vector<std::string> required_target_dependent_kinds(const Scenario&)`,
  `ClaimClass derive_claim(const ScenarioDocument&, const std::vector<TraceDocument>& release,
  const std::vector<TraceDocument>& reconstruction)`, and `ClaimClass claim` on
  `ComparisonReport`.

- [ ] **Step 1: Write the failing tests**

`test_compare.cpp`: `claim_is_surface_when_only_lifecycle_hooks_are_required`,
`claim_is_behavioural_when_a_required_save_site_was_observed_on_both_sides`,
`claim_stays_surface_when_the_required_site_was_never_observed`,
`claim_is_surface_when_the_save_site_is_optional` (the `safe-mode-smoke` shape), and
`report_carries_the_derived_claim`. Traces are built in memory through `TraceWriter` as the
corpus does.

- [ ] **Step 2: Run to verify failure**

Run: `ctest --test-dir harness/build -R records` — expected: no such test; build fails on
`derive_claim`.

- [ ] **Step 3: Implement the derivation**

`required_target_dependent_kinds` walks `scenario.required_hooks` against a documented registry
containing exactly one entry today — `GeometryDash.exe:rva:0x16c8b0` → `"save"` — and returns
`"save"` only when that key is required. `derive_claim` returns `surface` when the list is
empty or when either variant's events contain no record of that kind, else `behavioural`.
`compare_traces` sets `report.claim` before any early return. Comment the omission: the second
half of the rule ("a required error code") has no member in `mh-scenario/1`, and adding one is a
schema change that is not made here.

- [ ] **Step 4: Run the tests**

Run: `ctest --test-dir harness/build -R compare --output-on-failure` — expected: pass.

- [ ] **Step 5: Print it only when a pass exists**

In `trace_compare.cpp`, after the status line, print
`claim: <class>` plus one fixed qualifier line — for `surface`:
`  note: lifecycle surface only; not evidence that the reconstruction behaves like the release`.
Guard both behind `pass`/`pass_with_warnings`.

- [ ] **Step 6: Verify by hand**

Run: `harness\build\mh_trace_compare.exe harness\scenarios\safe-mode-smoke.json --release <one
trace> --reconstruction <one trace>` and confirm a non-pass status prints no `claim:` line.

- [ ] **Step 7: Commit**

`git commit -m "feat(compare): derive and report the claim a pass can support"`

---

### Task 3: Placeholder digests are blocked, not invalid

**Files:**
- Modify: `harness/include/mh/comparator.hpp:80-90`, `harness/src/comparator.cpp:282-390`
- Test: `harness/tests/test_comparator.cpp`

**Interfaces:**
- Produces: `GateIssue` gains `GateIssueSeverity severity` (default `invalid`), plus
  `bool is_placeholder_digest(std::string_view)` in the comparator; `GateResult.status` is
  `invalid` if any issue is `invalid`, else `blocked` if any is `blocked`.

- [ ] **Step 1: Write the failing tests**

`comparator_gate_blocks_a_placeholder_reconstruction_digest`,
`comparator_gate_blocks_a_placeholder_harness_digest`,
`comparator_gate_blocks_a_level_row_where_both_sides_are_placeholders`,
`comparator_gate_still_invalidates_two_real_digests_that_differ`, and
`comparator_gate_reports_the_placeholder_as_its_own_finding` (the detail names the field).

- [ ] **Step 2: Run to verify failure**

Run: `ctest --test-dir harness/build -R comparator` — expected: fail, `invalid` where `blocked`
is expected.

- [ ] **Step 3: Implement**

`is_placeholder_digest` = 64 chars, all `'0'`. In the target, harness and level rows: if either
side is a placeholder, add a `blocked` issue `"<row>: the declared digest is a placeholder; the
evidence is not there yet"` and skip the equality check for that row. Settings and scenario rows
are unchanged.

- [ ] **Step 4: Run the tests**

Run: `ctest --test-dir harness/build --output-on-failure` — expected: all green.

- [ ] **Step 5: Commit**

`git commit -m "fix(comparator): report a placeholder digest as blocked, not invalid"`

---

### Task 4: The reconstruction mod

**Files:**
- Create: `reconstruction/geode/mod.json`, `reconstruction/geode/CMakeLists.txt`,
  `reconstruction/geode/src/main.cpp`
- Modify: `.gitignore` (build output), `harness/observer/src/main.cpp:227-248`
  (`build_mods_list` recognises `megahack.reconstruction`)

**Interfaces:**
- Produces: a packaged `megahack.reconstruction.geode` containing `megahack.reconstruction.dll`,
  with `$modify` detours on `0x3B8EB0`, `0x3B4BA0`, `0x397F40`, `0x3981D0`, `0x16C8B0` and
  `PlayLayer::init`.

- [ ] **Step 1: Write `mod.json`**

`id: megahack.reconstruction`, `name: Mega Hack Reconstruction (target)`, `version: v0.0.1`,
`geode: 5.11.2`, `gd.win: 2.2081`, description stating it installs no behaviour and writes no
trace.

- [ ] **Step 2: Write `CMakeLists.txt`**

The observer's recipe verbatim (`add_subdirectory("$ENV{GEODE_SDK}" ...)`, `include
"$ENV{GEODE_SDK}/cmake/GeodeFile.cmake"`, `setup_geode_mod(reconstruction DONT_INSTALL)`), with
one target `reconstruction` from `src/main.cpp` and no `mh` sources: it builds no records
(Task 1 note: the observer is the only emitter).

- [ ] **Step 3: Write `src/main.cpp`**

Five `$modify` classes that call the original and nothing else, each with a comment giving the
RVA and its note source. `$on_mod(Loaded)` logs `reconstruction target loaded` once.

- [ ] **Step 4: Build and package**

Run: `cd reconstruction\geode; geode build` (twice if the first exits 1). Confirm
`build/RelWithDebInfo/megahack.reconstruction.dll` and `build/megahack.reconstruction.geode`
exist and that `megahack.reconstruction.dll` is the name inside the archive.

- [ ] **Step 5: Record the artifact digest**

`sha256` the `.geode`'s inner DLL — that is what `--target` will name and what the manifest
pins. Build twice and compare, recording whether the build is reproducible.

- [ ] **Step 6: Commit**

`git commit -m "feat(reconstruction): add the megahack.reconstruction target mod"`

---

### Task 5: Reconcile the manifest with the artifacts

**Files:**
- Modify: `harness/scenarios/safe-mode-smoke.json:5-12`, `harness/tests/test_scenario.cpp`
- Modify: `harness/tools/preflight.py` only if a refusal needs the new role path

- [ ] **Step 1: Fill the two real digests**

`harness.sha256` = the built `megahack.trace.geode` (Task 1 build);
`targets.reconstruction.sha256` = the digest recorded in Task 4 Step 5. Leave
`level.sha256` zero and `level.fixture` as it is.

- [ ] **Step 2: Declare the baseline instead of leaving it ambiguous**

Extend `description` to say the run is on Geode 5.11.2 per `runtime_baseline`, while `baseline`
names the static analysis reference (5.6.1). Add
`scenario_accepts_a_baseline_that_differs_from_the_runtime` asserting the document stays valid.

- [ ] **Step 3: Run the gates**

`python harness\tools\test_preflight.py`, `python harness\tools\test_grade_trace.py`,
`ctest --test-dir harness/build --output-on-failure`,
`python scripts\check_doc_links.py`, `python scripts\make_progress_svg.py --check`.

- [ ] **Step 4: Commit**

`git commit -m "chore(scenarios): pin safe-mode-smoke to the built artifacts"`

---

### Task 6: One comparison over real runs (needs the user)

- [ ] **Step 1: Install the release target** into `runtime511` `mods/`, remove
  `megahack.reconstruction.geode` from that folder, install `megahack.trace.geode`.
- [ ] **Step 2: Build the release context** with `preflight.py --role release --expect-sha256
  1635c339…` and run twice (`--repetition 0`, `1`); grade each with `grade_trace.py`.
- [ ] **Step 3: Swap targets** — remove the release, install `megahack.reconstruction.geode`, and
  repeat with `--role reconstruction --expect-sha256 <reconstruction digest>`.
- [ ] **Step 4: Read the observer log** for `multiple_targets_loaded` and `hook_install_failed` on
  each run, and confirm `meta.mods` names the target that was installed.
- [ ] **Step 5: Compare** and record the status, the claim (or its absence) and the exact reason
  the gate stopped.
- [ ] **Step 6: Write `notes/reconstruction-variant.md`** with question, method, evidence,
  conclusion and open questions; record what was decided and what was deliberately not done.
