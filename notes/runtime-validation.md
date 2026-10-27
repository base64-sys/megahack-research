# Runtime validation: trace contract and scenario manifest

- **Question** - how can a runtime trace of the release and the reconstruction be
  compared without treating two similar-looking logs as equivalent behaviour?
- **Baseline** - Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1 static reference;
  runtime validation uses Geode 5.11.2 in the separate `runtime511` profile.
- **Runtime baseline** - Geode 5.11.2, GD 2.2081, Windows x86-64.
- **Ground truth** - `samples/absolllute.hackmega/absolllute.hackmega_original.dll`,
  SHA-256 `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`,
  8,697,056 bytes, image base `0x180000000`.
- **Status** - contract only. No trace has been captured, no game process has been
  launched, and no cluster is runtime-validated by this note. Fields marked
  `UNVERIFIED` need a run and, where noted, a pinned-SDK check before they can be used
  as evidence.

## Runtime baseline decision (2026-10-27)

The static reference remains Mega Hack v9.1.3 / GD 2.2081 / Geode 5.6.1. The local
runtime environment is **Geode 5.11.2**, because the release mod is installed and the
user reports that it loads and runs there. That is compatibility evidence, not a 5.6.1
trace and not a re-derivation of the static notes.

A separate local profile named `runtime511` was created from a copy of the installed
game, excluding the original `geode`, `replays`, and player-data files. It contains the
same 5.11.2 loader and, at this stage, only the `geode.node-ids` dependency. The original
default profile (`a`) still has 80 `.geode` packages and was not modified. The copied
`Geode.dll` has SHA-256
`0C34F50454C102637038CAE48C8C7DD685DA3582FE474358024B1EB6833684D3` (file version
5.11.2), and the copied `GeometryDash.exe` has SHA-256
`FC5A16C292278BC2E8E078FB1D5023C2BD658322DD72712767EA70C2DD9EC6D0`.

Every trace must record `environment.geode.version` as `5.11.2` and its hash. A
comparison is valid only within the same runtime baseline. A pass under 5.11.2 is
evidence about 5.11.2; it must not be silently quoted as 5.6.1 evidence. If the project
later restores a 5.6.1 profile, the scenario must be re-run there rather than merging
the two sets.

The profile has not been launched yet. The loader copy is verified by file version and
hash; actual loading is a Step 2 check.

## What already exists, and what it does not establish

The static side is substantially complete. The runtime side is not.

| Surface | Evidence | What it provides | What it does not provide |
| --- | --- | --- | --- |
| Hook sites | `recovered/functions/_hook_sites.tsv`; [hook-dispatch.md](hook-dispatch.md) | 338 registration sites over 215 distinct GD methods; registrar VA, method, RVA or name, callback, callback kind, helper, raw order | No runtime arguments, returns, call order, thread, or decision |
| Tag bindings | [hook-registrations.md](hook-registrations.md) | 125 tag registrations, 186 same-image hooks across 111 callbacks, callback and detour VAs | No proof that a callback ran, or what it did |
| Thunks | `recovered/functions/_wrappers.tsv`; [hook-dispatch.md](hook-dispatch.md) | 277 thunk rows and hook-target RVAs; a second, independent angle on host addresses | No runtime invocation evidence |
| Settings | `recovered/functions/_settings.tsv`; [settings-registry.md](settings-registry.md) | Accessor-verified tag rows and types | No shipped defaults for every tag, no runtime snapshot |
| Feature state | [feature-inventory.md](feature-inventory.md) | 106 of 389 changelog rows were `mapped` on 2026-10-26 | The other rows are `cluster`; mapping is not behavioural validation |
| Cluster status | [m3-validation.md](m3-validation.md) | All five sampled clusters are `UNVERIFIED - structural only` | No release-versus-reconstruction comparison |
| Host ABI | [host-abi.md](host-abi.md); [hook-dispatch.md](hook-dispatch.md) | `GetModuleHandleA(NULL) + RVA` path, verified RVA examples, `GJGameLevel::savePercentage` veto | No runtime tracing of those paths |
| Persistence | [persistence.md](persistence.md); [save-lifecycle.md](save-lifecycle.md) | `hackpro.json`, GD `data.json`/`save.json`; the only storage-side hook is the `savePercentage` veto; `onQuit` is a key replay, not a flush | No hook-boundary file-write event; save files need external observation |
| Reconstruction | `reconstruction/tests/test_reconstruction.cpp` | Nine host-side C++ tests for entry, settings, listeners, serialization, actions | No Geode-loaded build, no GD hook, no runtime trace |
| Skeleton | `reconstruction/geode_skeleton/mod.json`; `reconstruction/CMakeLists.txt` | Geode 5.6.1 / GD 2.2081 skeleton, `megahack.skeleton` v0.0.1 | It is a compile-time stub, not the harness |

The release manifest is `absolllute.hackmega` v9.1.3, `early-load: true`, with a
`geode.node-ids >= v1.22.0` dependency. The skeleton is `megahack.skeleton` v0.0.1,
also `early-load`. A harness must be a third mod and must not reuse either id.

## Non-goals

This contract does not compare pixels, audio output, frame rate, input latency, or
anti-cheat behaviour. It does not recover exact control flow inside a callback unless
the callback itself is observed. It does not use an online level, account, leaderboard,
or any other external service. It does not improve gameplay or bypass a licence or
entitlement check.

## Trace format

A trace is UTF-8 JSON Lines (JSONL): one complete JSON object per line, no comments,
no BOM. Every line carries `"schema": "mh-trace/1"`. Unknown fields are allowed for
forward compatibility; a change to the meaning of an existing field requires a new
schema string.

There are three record types:

| `type` | Written by | Purpose |
| --- | --- | --- |
| `meta` | harness, first line | Run identity, hashes, scenario, environment, settings, level, harness identity |
| `event` | harness | Hooks, milestones, save observations, errors |
| `end` | harness, last line | Termination status, counts, dropped events, artifact hashes |

A trace without an `end` record is **incomplete**, not a successful truncated trace.
A trace whose `seq` has a gap, or whose `end.counts.dropped` is non-zero, is **not
eligible for a pass** unless the dropped events are proven optional by the manifest.

### Common fields

| Field | Type | Meaning |
| --- | --- | --- |
| `schema` | string | Always `mh-trace/1` |
| `type` | string | `meta`, `event`, or `end` |
| `run_id` | UUID string | Random identity of this file; never used for matching |
| `run_fingerprint` | 64-hex string | Deterministic identity: scenario, target, harness, settings, level, repetition |
| `seq` | integer | Monotonic per file, starting at 0 for `meta`; gaps indicate loss |
| `t_ns` | integer | Monotonic `steady_clock` nanoseconds at capture; not comparable across runs |
| `frame` | integer or null | Frame counter if observed; `null` plus `capture` entry if not |
| `thread` | object | `{ "id": "<opaque>", "role": "main"|"render"|"audio"|"worker"|"unknown" }` |
| `capture` | object | `{ "status": "complete"|"partial"|"unavailable", "missing": [paths], "reasons": {path: reason} }` |

`seq` is assigned before the write is attempted. `end.counts.attempted` and
`end.counts.dropped` make a write failure visible; a gap without that explanation is
invalid.

`run_fingerprint` is SHA-256 over canonical JSON (UTF-8, sorted keys, no insignificant
whitespace) of:

```
{scenario.sha256, target.sha256, harness.sha256, settings.tags, level.sha256, repetition_index}
```

It is the deterministic identity used to group runs. `run_id` is deliberately random so
two files cannot collide even if a runner restarts.

Every key defined by this schema is present even when its value is null. An absent key
is invalid. In an event value, a real null is `{"state":"null"}`; a top-level metadata
field may be JSON `null` only when `capture.missing` names it and `capture.reasons`
explains why.

`capture.status` has three values:

| Status | Meaning |
| --- | --- |
| `complete` | Every field this record needs for its role was observed |
| `partial` | Some fields were observed and some were not; `missing` lists them |
| `unavailable` | No usable observation, or a required field is absent; the record cannot establish its claim |

The comparability gate, not the record alone, decides whether a partial field is
required. A `meta` record with a missing GD hash can be `partial`; the environment is
still incomplete for an M3 pass.

`thread.id` is opaque within a run. `thread.role` is `"main"` only when the harness can
prove it is the GD main thread (for example by comparing the id captured at
`PlayLayer::init` with the current id); otherwise it is `"unknown"`. Do not guess a role
from a thread name.

### `meta` record

```json
{"schema":"mh-trace/1","type":"meta","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":0,"t_ns":0,"created_utc":"2026-10-27T12:00:00Z","target":{"role":"release","artifact":"absolllute.hackmega_original.dll","sha256":"1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92","size":8697056,"image_base":"0x180000000"},"scenario":{"id":"safe-mode-smoke","sha256":"0000000000000000000000000000000000000000000000000000000000000000"},"environment":{"os":"Windows 10 22H2","arch":"x86_64","gd":{"version":"2.2081","sha256":null},"geode":{"version":"5.11.2","sha256":null},"harness":{"id":"megahack.trace","version":"0.1.0","sha256":"0000000000000000000000000000000000000000000000000000000000000000"}},"mods":[{"id":"absolllute.hackmega","role":"target","version":"v9.1.3","sha256":"1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92"},{"id":"megahack.trace","role":"observer","version":"0.1.0","sha256":"0000000000000000000000000000000000000000000000000000000000000000"}],"settings":{"reset":"defaults","tags":[{"tag":"0x1E2","label":"HACK_SAFE_MODE","value":true}],"snapshot_sha256":"0000000000000000000000000000000000000000000000000000000000000000"},"level":{"fixture":"fixtures/runtime/safe-mode-smoke.gmd","sha256":"0000000000000000000000000000000000000000000000000000000000000000","source":"generated"},"clock":{"source":"steady_clock","unit":"ns"},"capture":{"status":"partial","missing":["environment.gd.sha256","environment.geode.sha256"],"reasons":{"environment.gd.sha256":"not hashed by the runner in this example","environment.geode.sha256":"not hashed by the runner in this example"}}}
```

`target.role` is `"release"` or `"reconstruction"`. `target.sha256` is the hash of the
DLL actually loaded, not a path. `target.image_base` is the preferred image base from
the PE, not the runtime base; the runtime base belongs in `hook.va` only.

`settings.snapshot_sha256` is the hash of the settings file(s) for **that variant**. It
is evidence of a reproducible local state; it is not compared across release and
reconstruction, because the two implementations may serialize the same semantic
settings differently. The semantic `settings.tags` list is the cross-variant identity.

`level.fixture` is a local fixture, never a user level and never an online level. A
fixture may be generated by a tracked script or supplied locally; it must have a stable
SHA-256. `source` is `"generated"` or `"local-fixture"`; `"user-level"` is not allowed
for a tracked scenario.

`capture` on `meta` distinguishes an absent hash from a hash that happens to be null.
A `null` hash with no `capture.reasons` entry is invalid. For M3 validation, a missing
GD or Geode hash makes the environment **incomplete**; see [Comparability gate](#comparability-gate).

### `event` record

Common event fields are `schema`, `type`, `run_id`, `run_fingerprint`, `seq`, `t_ns`,
`frame`, `thread`, `kind`, and `capture`. `kind` selects the payload:

| `kind` | Payload | Purpose |
| --- | --- | --- |
| `hook` | `phase`, `hook`, `call`, `feature` | A GD method or target callback was entered or exited |
| `milestone` | `name`, `data` | A scenario boundary such as level start, death, completion, save request. `data` is a value object: it holds what was observed at the site, or the model's null value when nothing was. A level name is player data, so `level_started` records it as `redacted` (or `unavailable` when no level was passed), never as a value |
| `save` | `store`, `phase`, `source`, `path_template`, `exists`, `size`, `sha256`, `keys` | An observed file state or write; not inferred from a hook |
| `error` | `code`, `message`, `fatal`, `hook_key` | Harness, hook-install, scenario, or target error |

The `error.code` vocabulary is `hook_install_failed`, `signature_unavailable`,
`trace_write_failed`, `target_missing`, `multiple_targets_loaded`,
`scenario_mismatch`, `capture_overflow`, `level_load_failed`, `timeout`, and
`game_crash`. An unknown code is allowed but must be treated as fatal until classified.
`fatal: true` means the run cannot produce a pass.

`feature` is an optional array of tags observed for this event:
`[{"tag":"0x1E2","label":"HACK_SAFE_MODE","key":null,"enabled":true,"source":"settings"}]`.
`key` is the settings key if it is known from the evidence; `null` means the key is not
established, not that the setting has no key. `enabled` is the value read at the hook,
not the manifest's requested value.

#### Hook identity

```json
{"phase":"enter","hook":{"observer":"gd_method","key":"GeometryDash.exe:rva:0x3b8eb0","module":"GeometryDash.exe","method":"PlayLayer::resetLevel","resolution":"rva","rva":"0x3B8EB0","va":"0x7FF6A1B8EB0","target_name":null,"target":{"module":"absolllute.hackmega_original.dll","callback_va":"0x180199E80","callback_rva":"0x199E80","registration_va":"0x180199D10","tag":"0x1E2","tag_label":"HACK_SAFE_MODE","order_raw":"0xffffffe0","order_signed":-32,"priority":null},"convention":"unknown"},"call":{"call_id":"c1","depth":1,"parent_call_id":null,"this":{"state":"observed","type":"ptr","value":"0x1F4A2B00000"},"args":{"state":"observed","items":[]},"return":{"state":"null"}},"feature":[{"tag":"0x1E2","label":"HACK_SAFE_MODE","key":null,"enabled":true,"source":"settings"}],"capture":{"status":"complete"}}
```

The stable identity is `hook.key`:

- RVA-resolved target: `<module>:rva:<lowercase 0x hex RVA>`
- Name-resolved target: `<module>:name:<exact method string>`

`hook.va` is the runtime address and is **never** used for matching. `hook.module` is
the module that contains the observed function; `hook.target.module` is the Mega Hack
or reconstruction module that registered the callback. They are different modules and
must not be conflated.

`resolution` is `"rva"` or `"name"`. For `"name"`, `rva` and `va` are null and
`target_name` is the exact string from `_hook_sites.tsv`/`_host_methods.tsv`; a harness
that cannot resolve the name records the hook as unavailable rather than inventing an
RVA.

`target.order_raw` is the raw order value from `_hook_sites.tsv`; `target.order_signed`
is its signed 32-bit interpretation (`0xFFFFFFE0` and `-32` are the same value). The
value is evidence, not an instruction to infer priority semantics. `target.priority` is
the runtime priority reported by the harness only if the harness owns the hook and can
read it from `geode::Hook`; a target mod's hook object may not be enumerable, in which
case `priority` is null with a `capture` entry.

`convention` is `"unknown"` unless the harness can read it. The release uses a
`TulipConvention` in the registration helpers, but `_hook_sites.tsv` does not carry a
per-site convention, so the trace must not claim one.

#### Call fields

`call.call_id` is unique within a run. `call.depth` is the nesting depth on that thread,
starting at 1 for the outermost observed call; `parent_call_id` is the enclosing
observed call, or null. `call.this` is a value object; for a free function it is
`{"state":"null"}`. `args` is either
`{"state":"observed","items":[value, ...]}` or `{"state":"unavailable","reason":"..."}`.
`return` is a value object. On `phase:"exit"`, `call.duration_ns` may be present; it is
excluded from cross-run comparison because it is wall-clock dependent.

A hook enter without a matching exit is an incomplete call: record it at `end` as
`counts.unclosed`, and do not silently close it. A call id reused within a run is
invalid.

#### Value objects

Every captured value is a tagged object, so an unavailable value cannot be confused
with a real null:

| Object | Meaning |
| --- | --- |
| `{"state":"observed","type":"u64","value":"0x180199E80"}` | 64-bit unsigned; hex string |
| `{"state":"observed","type":"i32","value":-1}` | 32-bit signed; JSON number |
| `{"state":"observed","type":"f32","value":0.5,"bits":"0x3F000000"}` | Float; `bits` is authoritative, `value` is convenience |
| `{"state":"observed","type":"bool","value":true}` | Boolean |
| `{"state":"observed","type":"str","value":"..."}` | UTF-8 string |
| `{"state":"observed","type":"bytes","value":"<base64>","encoding":"base64"}` | Opaque bytes; do not put user data here |
| `{"state":"observed","type":"ptr","value":"0x..."}` | Pointer; excluded from equality by default |
| `{"state":"null"}` | Observed null or a void result |
| `{"state":"unavailable","reason":"..."}` | Not captured; never equal to null |
| `{"state":"redacted","reason":"..."}` | Deliberately withheld; never equal to null |

Floats always carry `bits`; `value` is not used for equality when `bits` is present.
Pointers are recorded for correlation but are excluded from equality because heap and
module bases vary. A pointer that is the **subject** of a scenario (for example a
verified object identity) needs a manifest-specific rule; the default is exclusion.
Strings that contain a player name, account identifier, level name, or chat text are
`redacted`; the trace records the reason and never the value. A save file's contents are
represented by size, key count and SHA-256, not by the JSON itself.

#### Save events

```json
{"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":4,"t_ns":900,"frame":240,"thread":{"id":"main","role":"main"},"kind":"save","store":"hackpro","phase":"snapshot_after","source":"filesystem","path_template":"<mod_config>/hackpro.json","exists":true,"size":12345,"sha256":"0000000000000000000000000000000000000000000000000000000000000000","keys":202,"capture":{"status":"complete"}}
```

`store` is `hackpro`, `gd_data`, `gd_save`, `mod_save`, `unknown`, or `other`.
`phase` is `snapshot_before`, `read`, `write`, `flush`, or `snapshot_after`.
`source` is `harness`, `filesystem`, `process_monitor`, or `runner`.

A `save` event must come from an actual file observation. It is **not** inferred from
`GJGameLevel::savePercentage` or `PlayLayer::onQuit`; those are hook events. The release's
save lifecycle does no file I/O at the hook boundary ([save-lifecycle.md](save-lifecycle.md)),
and the only storage-side hook is a veto ([persistence.md](persistence.md)). External
observation is therefore required for a real save event.

`path_template` is a class, not a literal user path: `<mod_config>/hackpro.json`,
`<local_appdata>/Octavus/data.json`, `<local_appdata>/Octavus/save.json`. An actual
absolute path, if captured for debugging, goes in a `local_path` field that is excluded
from comparison and from the evidence record.

#### `end` record

```json
{"schema":"mh-trace/1","type":"end","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":5,"t_ns":1000,"status":"completed","exit_code":0,"frames":240,"duration_ns":1000,"counts":{"events":4,"attempted":4,"dropped":0,"unclosed":0,"hook_enter":1,"hook_exit":1,"errors":0},"artifacts":{"stdout_sha256":null,"stderr_sha256":null,"minidump_sha256":null},"capture":{"status":"partial","missing":["artifacts.stdout_sha256"],"reasons":{"artifacts.stdout_sha256":"not collected in this example"}}}
```

`status` is `completed`, `stopped`, `crashed`, `timeout`, or `harness_error`. A crash
or timeout is a failed run, not a successful comparison. `counts.attempted` is the
number of **event** records the harness attempted to write; meta and end are excluded.
`counts.dropped` is the number of attempted events not written. `counts.events` is the
number actually written. `counts.unclosed` is the number of hook enters without exits.

`artifacts` contains hashes of sidecars, never a self-hash of the trace file. The runner
writes a `<trace>.sha256` sidecar after the process exits; the evidence record cites
that sidecar. A minidump or raw log is untracked and is represented by a hash plus a
local path that does not enter the index.

### Complete illustrative trace

The following seven lines are one valid JSONL trace. Hashes and counts are placeholders;
this is a format example, not a captured run. It deliberately shows a save observation
whose `size`, `sha256` and `keys` are **unavailable**, which is different from a real
`null`.

```jsonl
{"schema":"mh-trace/1","type":"meta","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":0,"t_ns":0,"target":{"role":"release","artifact":"absolllute.hackmega_original.dll","sha256":"1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92","size":8697056,"image_base":"0x180000000"},"scenario":{"id":"safe-mode-smoke","sha256":"0000000000000000000000000000000000000000000000000000000000000000"},"environment":{"os":"Windows 10 22H2","arch":"x86_64","gd":{"version":"2.2081","sha256":null},"geode":{"version":"5.11.2","sha256":null},"harness":{"id":"megahack.trace","version":"0.1.0","sha256":"0000000000000000000000000000000000000000000000000000000000000000"}},"settings":{"reset":"defaults","tags":[{"tag":"0x1E2","value":true}],"snapshot_sha256":"0000000000000000000000000000000000000000000000000000000000000000"},"level":{"fixture":"fixtures/runtime/safe-mode-smoke.gmd","sha256":"0000000000000000000000000000000000000000000000000000000000000000","source":"generated"},"capture":{"status":"partial","missing":["environment.gd.sha256","environment.geode.sha256"],"reasons":{"environment.gd.sha256":"not hashed","environment.geode.sha256":"not hashed"}}}
{"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":1,"t_ns":100,"frame":0,"thread":{"id":"main","role":"main"},"kind":"milestone","name":"level_started","data":{"state":"null"},"capture":{"status":"complete"}}
{"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":2,"t_ns":150,"frame":0,"thread":{"id":"main","role":"main"},"kind":"milestone","name":"attempt_started","data":{"state":"null"},"capture":{"status":"complete"}}
{"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":3,"t_ns":200,"frame":1,"thread":{"id":"main","role":"main"},"kind":"hook","phase":"enter","hook":{"observer":"gd_method","key":"GeometryDash.exe:rva:0x3b8eb0","module":"GeometryDash.exe","method":"PlayLayer::resetLevel","resolution":"rva","rva":"0x3B8EB0","va":"0x7FF6A1B8EB0","target_name":null,"target":{"module":"absolllute.hackmega_original.dll","callback_va":"0x180199E80","callback_rva":"0x199E80","registration_va":"0x180199D10","tag":"0x1E2","tag_label":"HACK_SAFE_MODE","order_raw":"0xffffffe0","order_signed":-32,"priority":null},"convention":"unknown"},"call":{"call_id":"c1","depth":1,"parent_call_id":null,"this":{"state":"observed","type":"ptr","value":"0x1F4A2B00000"},"args":{"state":"observed","items":[]},"return":{"state":"null"}},"capture":{"status":"complete"}}
{"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":4,"t_ns":250,"frame":1,"thread":{"id":"main","role":"main"},"kind":"hook","phase":"exit","hook":{"observer":"gd_method","key":"GeometryDash.exe:rva:0x3b8eb0","module":"GeometryDash.exe","method":"PlayLayer::resetLevel","resolution":"rva","rva":"0x3B8EB0","va":"0x7FF6A1B8EB0","target_name":null,"target":{"module":"absolllute.hackmega_original.dll","callback_va":"0x180199E80","callback_rva":"0x199E80","registration_va":"0x180199D10","tag":"0x1E2","tag_label":"HACK_SAFE_MODE","order_raw":"0xffffffe0","order_signed":-32,"priority":null},"convention":"unknown"},"call":{"call_id":"c1","depth":1,"duration_ns":50,"return":{"state":"null"}},"capture":{"status":"complete"}}
{"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":5,"t_ns":300,"frame":1,"thread":{"id":"main","role":"main"},"kind":"save","store":"hackpro","phase":"snapshot_after","source":"filesystem","path_template":"<mod_config>/hackpro.json","exists":true,"size":null,"sha256":null,"keys":null,"capture":{"status":"unavailable","missing":["size","sha256","keys"],"reasons":{"size":"file locked","sha256":"file locked","keys":"file locked"}}}
{"schema":"mh-trace/1","type":"end","run_id":"00000000-0000-4000-8000-000000000001","run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000","seq":6,"t_ns":400,"status":"completed","exit_code":0,"frames":1,"duration_ns":400,"counts":{"events":5,"attempted":5,"dropped":0,"unclosed":0,"hook_enter":1,"hook_exit":1,"errors":0},"capture":{"status":"partial","missing":["environment.gd.sha256"],"reasons":{"environment.gd.sha256":"not hashed"}}}
```

## Scenario manifest

A scenario is a tracked JSON file under `runtime-scenarios/` (created when the harness
exists). The trace's `scenario.sha256` is the SHA-256 of the manifest file bytes, so the
manifest must not contain its own hash.

```json
{
  "schema": "mh-scenario/1",
  "id": "safe-mode-smoke",
  "status": "draft",
  "description": "Short local level; enable Safe Mode; verify the savePercentage veto path and the reset/postUpdate lifecycle.",
  "baseline": {"mega_hack": "v9.1.3", "gd": "2.2081", "geode": "5.6.1", "arch": "x86_64"},
  "runtime_baseline": {"geode": "5.11.2", "profile": "runtime511"},
  "targets": {
    "release": {"artifact": "absolllute.hackmega_original.dll", "sha256": "1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92"},
    "reconstruction": {"artifact": "megahack.reconstruction.dll", "sha256": "0000000000000000000000000000000000000000000000000000000000000000"}
  },
  "harness": {"id": "megahack.trace", "version": "0.1.0", "sha256": "0000000000000000000000000000000000000000000000000000000000000000"},
  "mods": {"required": ["absolllute.hackmega", "megahack.trace"], "dependency_versions": {"geode.node-ids": ">=v1.22.0"}, "other_mods": "forbidden"},
  "level": {"fixture": "fixtures/runtime/safe-mode-smoke.gmd", "sha256": "0000000000000000000000000000000000000000000000000000000000000000", "source": "generated", "notes": "Local fixture only; no user or online level."},
  "profile": {"isolation": "disposable", "notes": "Use a disposable Windows profile or VM; restore from a snapshot. Do not run against a live player profile."},
  "settings": {"reset": "defaults", "tags": [{"tag": "0x1E2", "label": "HACK_SAFE_MODE", "value": true}]},
  "input": {"driver": "harness", "actions": [{"at_frame": 30, "action": "press", "key": "space"}, {"at_frame": 60, "action": "release", "key": "space"}, {"at_frame": 180, "action": "quit", "key": null}]},
  "timing": {"max_frames": 240, "timeout_ms": 30000, "fps_cap": 60, "stop_on": ["save_requested", "player_died", "level_completed"]},
  "milestones": {"required": ["level_started", "attempt_started"], "optional": ["save_requested", "player_died", "level_completed"]},
  "hooks": {
    "required": [
      {"key": "GeometryDash.exe:rva:0x3b8eb0", "phases": ["enter", "exit"], "min_count": 1},
      {"key": "GeometryDash.exe:rva:0x3b4ba0", "phases": ["enter", "exit"], "min_count": 1},
      {"key": "GeometryDash.exe:rva:0x397f40", "phases": ["enter", "exit"], "min_count": 1},
      {"key": "GeometryDash.exe:rva:0x3981d0", "phases": ["enter", "exit"], "min_count": 1}
    ],
    "optional": [
      {"key": "GeometryDash.exe:rva:0x16c8b0", "phases": ["enter", "exit"], "min_count": 1, "note": "UNVERIFIED trigger: the local level must make GD request a percentage save."}
    ]
  },
  "repetitions": 2,
  "comparison": {
    "order_groups": [["level_started", "attempt_started"]],
    "tolerances": {},
    "volatile": ["t_ns", "hook.va", "thread.id", "call.this.value", "call.duration_ns", "save.local_path"]
  }
}
```

The manifest's `hooks.required` is the observation contract, not a claim that the
release calls every hook on every run. If a required hook is absent, the scenario is
invalid for that cluster; it is not a pass by omission.

`baseline` is the static reference (Mega Hack v9.1.3 / GD 2.2081 / Geode 5.6.1).
`runtime_baseline` is the environment that actually produced the trace; the comparability
gate uses the runtime baseline. Do not compare a 5.11.2 trace with a future 5.6.1 trace.

`harness` pins the observer build; a different harness build is a different observer and
invalidates comparison. `mods.required` lists the target and observer; `mods.other_mods`
is `"forbidden"` for a controlled scenario. The release has a
`geode.node-ids >= v1.22.0` dependency, so its resolved version and hash belong in the
trace's `mods` list. The `meta.mods` example is abbreviated; a real trace lists every
enabled mod, including dependencies, with version and SHA-256, or marks the field
`unavailable` with a reason.

`status` is `draft`, `approved`, or `retired`. Only `approved` scenarios may produce
M3 evidence. `draft` scenarios have unresolved trigger or fixture questions.

### Level and profile

The scenario must use a local level fixture with a stable hash. A generated fixture is
preferred because it is original work and can be regenerated. A user-created level is
not allowed, even if hashed. A downloaded level is not allowed.

GD and Mega Hack write into the user's local app-data. No Geode API for redirecting
GD's save directory has been verified. The safe default is a disposable Windows
profile or a VM snapshot, with a documented restore step. The runner must record
whether it used an isolated profile; a run against a live profile is **not eligible**
for M3 evidence. `UNVERIFIED:` whether a supported launch flag or environment variable
can redirect the GD save path on 2.2081.

### Input

`driver` is `"harness"` for input injected through the game's own input path, `"os"` for
OS-level synthetic input, or `"operator"` when a human presses the keys and quits. `"os"` is
less deterministic and must be declared in the manifest; it is not the default. `"operator"`
is the honest declaration while no input driver exists: the actions are instructions to the
operator, `at_frame` is a target rather than a promise, and the trace's authority for what was
actually pressed is the `pushButton`/`releaseButton` hook records, not `input_sent`. The exact
harness input API is `UNVERIFIED` for the 5.11.2 runtime; this contract defines only the
logical actions. An action has `at_frame`, `action` (`press`, `release`, `quit`, or `wait`),
and `key` (`space`, `mouse_left`, or null for `quit`).

The trace records the observed hook calls (`PlayerObject::pushButton`,
`PlayerObject::releaseButton`, `GJBaseGameLayer::handleButton`) and the milestone
`input_sent`; it does not claim that the game received the input unless the input hook
fires. A missing input hook is a scenario failure, not an invisible success.

### Timing and stopping

`max_frames` is a hard bound; `timeout_ms` is a wall-clock bound. `fps_cap` is the
requested cap, not a guarantee. `stop_on` may name milestones such as `level_completed`,
`player_died`, or `save_requested`; the runner stops at the first one. Every stop reason
must appear in `end.status` or a `milestone` record. A timeout is a failed run.

### Expected milestones and hooks

Milestones are scenario-level observations, not engine facts. The minimum vocabulary is
`level_started`, `attempt_started`, `input_sent`, `player_died`, `level_completed`,
`save_requested`, `save_completed`, `quit_requested`, and `run_end`. A milestone with
`data` uses the value-object rules; no player name, level name, or account identifier is
captured.

`order_groups` lists sequences that must appear in order on the main thread. Events
outside a group are unordered unless a hook entry says otherwise. This avoids
over-constraining render/audio concurrency while still pinning the scenario's causal
spine.

## Comparability gate

Two traces are comparable only when all of these hold. If any fails, the result is
`invalid` or `blocked`; do not compare them "approximately".

| Check | Required |
| --- | --- |
| Scenario | Same `scenario.sha256`, same `scenario.id` |
| Target | Expected role and SHA-256 for each side; release and reconstruction are separate runs |
| Harness | Same harness id, version, and SHA-256 |
| Runtime | Same GD version; same runtime Geode version (`5.11.2`) and profile; GD/Geode SHA-256 if the runner can hash them |
| Level | Same fixture SHA-256 and source |
| Settings | Same semantic `settings.tags` list; file hashes may differ by variant |
| Observation | Same observer mode, same required hook keys, same priority policy |
| Runs | At least two runs per variant, each with an `end` record and no crash/timeout |
| Completeness | No dropped required events, no unclosed required calls, no unavailable required fields |

A different GD or Geode build is a re-baselining, not a comparison. A different
harness build is a different observer and invalidates the comparison even if the target
hash is unchanged.

## Comparison rules

### 1. Normalize before matching

Remove fields that are not behaviour: `t_ns`, `hook.va`, `thread.id`,
`call.this.value` (unless the manifest explicitly makes an object identity meaningful),
`call.duration_ns`, `save.local_path`, absolute paths, and runtime module bases. Do not
remove a field merely because it differs between the two targets. A field is
nondeterministic only after it varies between repeated runs of the **same** target
(see [Stability](#8-stability)).

Canonical event keys:

| Event | Key |
| --- | --- |
| Hook | `hook.observer + "|" + hook.key + "|" + phase + "|" + call.depth` |
| Milestone | `"milestone|" + name` |
| Save | `"save|" + store + "|" + phase + "|" + source` |
| Error | `"error|" + code` |

The key is for grouping; occurrence indices are assigned only after alignment. Do not
use a runtime VA, timestamp, or thread id in a key.

### 2. Validate the call stack

Pair every hook enter with the next exit of the same `call_id` on the same thread.
`depth` must be consistent with the observed parent chain. An enter without an exit, an
exit without an enter, a reused `call_id`, or a depth jump is a trace error. A trace
with a call-stack error cannot produce a pass, even if the payloads look equal.

### 3. Match sequences per key

Before alignment, enforce each required hook's `min_count` on **both** sides. A
required key with zero events is missing, not a vacuous match. Then compare the ordered
payload sequences with a deterministic sequence alignment (longest common subsequence /
Needleman-Wunsch). A match requires equal canonical payloads within the declared
tolerances. A gap is a missing event; a
mismatched pair is a changed event. Tie-break by preferring an exact payload match,
then the earlier release event, then the earlier reconstruction event.

Per-key matching is intentional: it tolerates unrelated interleaving from render/audio
threads without pretending the global order is identical. Cross-key order is checked
separately by `order_groups` and by main-thread milestones.

### 4. Compare ordering

Compare the sequence of required milestone names and the hook-enter keys named in each
`order_groups` group with the same alignment. A required group that is out of order is a
failure. Events on threads whose role is not `main` are compared as multisets, not
ordered streams. If the harness cannot prove a thread is `main`, its events are not
eligible to satisfy an ordered group.

### 5. Compare values

| Type | Default comparison |
| --- | --- |
| Bool, enum, string, bytes hash | Exact |
| Integer | Exact |
| Float | Exact `bits`; tolerance only when declared for that field |
| Pointer | Excluded unless the manifest declares an identity rule |
| Null | Equal only to null |
| Unavailable | Never equal to anything; required unavailable is `blocked` |
| Redacted | Never equal to anything; required redacted is `blocked` |

A tolerance is a JSON Pointer in `comparison.tolerances`, for example:

```json
{"/call.args.items/0/value": {"abs": 0.0001, "rel": 0.000001, "why": "physics float cross-build rounding"}}
```

Tolerance is applied to the decoded `value`, not to the raw `bits`; `bits` remains the
exact default. The formula is
`abs(a-b) <= abs_tol + rel_tol * max(abs(a), abs(b))`. A tolerance without a `why` is
invalid. Do not apply a tolerance to a hash, a boolean, an enum, a
count, or a frame index. If a numeric value is deterministic within a variant, the
default is exact, even if a tolerance would hide a real difference.

### 6. Missing, extra, unavailable

| Finding | Required event | Optional event |
| --- | --- | --- |
| Missing in release or reconstruction | Fail | Warning |
| Extra in release or reconstruction | Fail | Warning |
| Capture unavailable | Blocked | Warning, if the manifest allows it |
| Dropped event | Blocked | Warning only if explicitly allowed |
| Unclosed call | Fail | Fail |

A `null` value is not a missing value. A `null` return is a valid observation for a
void function; an `unavailable` return means the harness could not observe it and the
required field is not satisfied.

### 7. Crashes and incomplete traces

A crash in either variant is a **failure**, not evidence of equivalence. Record
`end.status` as `crashed` when the harness can; when the process dies before `end`, the
runner records the missing end and the last event sequence. Preserve the faulting
module and RVA if available, the exception code, and a hash of any minidump. A timeout
is a failure. A trace with no `end` is incomplete. Never compare the surviving prefix
of a crashed run with a complete run as if it were a shorter scenario.

### 8. Stability

Before comparing release with reconstruction, compare each variant's repeated runs with
the same rules. If a required event, payload, or order differs within a variant, the
scenario is `unstable`; do not proceed to a cross-variant claim. Record the varying
field in `comparison.volatile` only after it has been observed to vary, with the
observed run ids. This prevents a real target difference from being excused as noise.

### 9. Result statuses

| Status | Meaning |
| --- | --- |
| `pass` | All required checks match; no warnings that affect required evidence |
| `pass_with_warnings` | Required checks match; only declared optional gaps remain |
| `fail` | A required event, value, order, or call-stack check differs |
| `blocked` | A required field or hook cannot be captured; no equivalence claim |
| `unstable` | A variant disagrees with itself across repetitions |
| `invalid` | Comparability gate failed (hash, version, scenario, observer, level, settings) |

Only `pass` supports an M3 runtime-validated claim. `pass_with_warnings` is useful
progress but is not enough to close the cluster criterion.

`unstable` now comes with a diagnostic: `mh_trace_compare` prints, per variant, the event keys
that disagree between run 1 and the other runs, and whether a key was absent or its payload
differed (capped at eight, with a note when the cap is hit). Those findings carry the `unstable`
severity, which `finalize_status` ignores, so the verdict is still decided only by `unstable` -
the diagnosis cannot turn a failure into a pass or the reverse.

## What can and cannot be captured at the hook boundary

| Field | Feasibility | Evidence / constraint |
| --- | --- | --- |
| Hook entry/exit for an RVA-resolved GD method | Yes, if the harness can install a Geode hook at `GD base + RVA` | `_hook_sites.tsv` has 308 RVA sites; `Mod::hook` takes an address. The Geode v5.11.1 hook API and `Hook::getPriority` are verified, but exact multi-mod ordering remains `UNVERIFIED` |
| Typed arguments and return | Yes for signatures the harness knows; otherwise no | A typed detour must match the original signature. Unknown callbacks cannot be safely declared; use `unavailable` |
| Raw registers for an unknown signature | Not assumed | A naked/register trampoline is not part of this contract; without a verified convention, capture only entry/exit |
| Hook address, priority, display name | Yes for a hook the harness owns | Geode v5.11.1 `Hook.hpp` has `getAddress`, `getDisplayName`, `getPriority`; whether a target mod's hook object can be enumerated is `UNVERIFIED`. Static `target.order_raw` remains the fallback |
| Target callback identity | Static from `_hook_sites.tsv` / `hook-registrations.md` | Callback VA and registration VA are evidence; they do not prove the callback ran |
| Whether a callback called the original | No, from a GD-method observer alone | Needs target-wrapper observation or branch tracing; do not infer from a GD call |
| Name-resolved hooks | Only with a verified resolver | 30 sites over 26 methods have no RVA; a harness that cannot resolve the name must mark them unavailable |
| Frame counter | `UNVERIFIED` | Geode/GD frame API is not yet confirmed for 5.11.2; fallback is `frame:null` |
| Thread role | `UNVERIFIED` | `std::this_thread::get_id` is available; proving "main" needs a known main-thread anchor |
| Save-file writes | No, at the Mega Hack hook boundary | The save lifecycle has no file I/O hooks; the only storage hook is the `savePercentage` veto. Use filesystem snapshots or Process Monitor externally |
| Save request / veto | Partial | Hook `GJGameLevel::savePercentage` entry and args; whether the original ran is not observable from the GD-method hook alone |
| Rendering output | No | Hook calls and args only; pixels/audio are out of scope |
| Exact branch coverage | No | Requires a debugger or instrumentation beyond Geode hooks |

**Verified against the SDK headers (2026-10-27).** The Geode `v5.11.1` tag's
[`Mod.hpp`](https://github.com/geode-sdk/geode/blob/v5.11.1/loader/include/Geode/loader/Mod.hpp)
has the `Mod::hook(void*, DetourType, std::string, TulipConvention, HookMetadata)`
overload, `Mod::getConfigDir`, `Mod::getTempDir`, and `Mod::getSaveDir`. The same tag's
[`Hook.hpp`](https://github.com/geode-sdk/geode/blob/v5.11.1/loader/include/Geode/loader/Hook.hpp)
has `Hook::getAddress`, `Hook::getDisplayName`, `Hook::getPriority`, and
`Hook::setPriority`; `Hook::create` documents that a hook is placed at the end of the
detour list by default and that metadata can control it. This confirms the API exists
in the pinned baseline. It does **not** establish the exact ordering and tie-break
between two mods' hooks, or that one mod can enumerate another mod's hooks.

The harness is a separate Geode mod (working id `megahack.trace`, `early-load: true`),
built against the matching Geode 5.11.x SDK rather than the compile-time stub in
`reconstruction/include/geode/entry.hpp`. The runner computes the target, harness and
fixture hashes, writes a run config, enables exactly one target mod, and supplies
`MH_RUN_CONFIG` and `MH_TRACE_PATH`. The harness reads the config with `std::ifstream`
and writes JSONL with `std::ofstream`; if either variable is absent or the config hash
does not match the manifest, it emits `scenario_mismatch` and ends with
`harness_error`. It must not fall back to a user save directory. The run config is
runner-generated and is copied into `meta`; it is not a third tracked format.

The harness may use only APIs verified against the matching Geode 5.11.x SDK. This note
does not invent a frame, input, file, or mod-enumeration API. Where an API is unknown,
the field is `unavailable`, not guessed. The runner, not the harness, is responsible
for asserting that release and reconstruction are never enabled together.

## M3 evidence record

A cluster is runtime-validated only when all of these are true:

1. The cluster has mapped static evidence: tag, registration/callback, and hook target
   are already recorded in `feature-inventory.md` and `hook-registrations.md`.
2. An `approved` scenario covers the cluster's required hooks and milestones.
3. Release and reconstruction each have at least two stable runs.
4. The comparability gate passes for every run pair.
5. The comparison status is `pass`; no required field is unavailable, no required
   event is missing, and no run crashed or timed out.
6. The raw traces remain untracked; the note records their SHA-256 sidecars, run ids,
   scenario SHA-256, target SHA-256 values, harness SHA-256, and the diff result.
7. A second person or a second independent tool invocation re-runs the comparison from
   the same trace files and obtains the same result.

Record it in [m3-validation.md](m3-validation.md) as:

| Cluster | Scenario | Manifest SHA-256 | Release runs | Reconstruction runs | Required events | Result | Trace SHA-256 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Input hooks | `input-smoke` | ... | 2 | 2 | ... | `pass` | ... |

A row with `blocked` or `pass_with_warnings` does not close the M3 criterion. Raw
traces, logs, and dumps go under `runtime-traces/` (gitignored); they are not committed.

## Open questions

- **Verified in the v5.11.1 headers:** `Mod::hook` accepts `HookMetadata`, and `Hook`
  exposes `getAddress`, `getDisplayName`, and `getPriority`. **Still UNVERIFIED:** the
  exact ordering and tie-break between two mods' hooks, and whether the observer can
  enumerate or read the target mod's hook priority.
- `UNVERIFIED:` can the observer select a priority that runs after the target's
  wrapper and still see the target's decision? `Hook::create` documents a default
  end-of-detour placement and metadata control, but that is not a complete ordering
  specification.
- `UNVERIFIED:` what is the 5.11.x API for the frame counter, main-thread identity, and
  synthetic input? The contract deliberately does not name one.
- `UNVERIFIED:` can GD 2.2081 be redirected to an isolated save directory with a
  supported launch flag or environment variable? Until confirmed, use a disposable
  profile or VM.
- `UNVERIFIED:` what exact level content triggers `savePercentage` after a short attempt. The
  Safe Mode example is a draft for this reason. The *container* is no longer open: the
  2.2081 save format and `k4` are recovered and implemented in
  [gd-save-format.md](gd-save-format.md); what remains is the minimal level-dict key set, a
  playable object string, and a run that proves GD loads the generated file.
- `UNVERIFIED:` can the harness observe the target's callback wrapper without
  reimplementing the release's `TulipConvention`? If not, only the GD-method boundary
  is observable, and internal branch claims stay out of scope.
- Name-resolved hooks (30 sites over 26 methods) have no RVA in `_hook_sites.tsv`.
  They are not covered by an RVA-based observer and need a separate resolver strategy.

## Consequences for the roadmap

This note supplies the method for the open M3 criterion "Reconstructed modules validated
against release behaviour for the sampled clusters", but it does not change any
cluster's status. The next step is a Geode 5.11.x harness that emits this format; until
then, `m3-validation.md` remains correct that all five clusters are structural-only.
