# megahack-research

Reverse-engineering workspace for **Mega Hack** v9.1.3, the Geode mod
`absolllute.hackmega` — a faithful source-level reconstruction of its behaviour, the
Geometry Dash host ABI it depends on, and the tooling that makes both verifiable, without
shipping or requiring Absolllute's binaries.

Research notes and original tooling only. [docs/GOALS.md](docs/GOALS.md) says what "done"
means, [docs/ROADMAP.md](docs/ROADMAP.md) tracks it, [AGENTS.md](AGENTS.md) holds the working
rules.

## Status

Four generated graphics, four lenses, because one number cannot portray progress honestly.

Where this stands: the release is statically mapped — 18,825 of 19,496 functions decompiled,
338 hook registrations over 215 GD methods and 125 tag bindings enumerated from instruction
bytes — and 28 of the roadmap's 33 exit criteria are met. The reconstruction is a Geode mod
(`megahack.reconstruction` v0.0.1) that reproduces the release's hook surface on the six
methods the observer watches; its own log line describes it exactly: `hook surface only, no
trace`. Runtime validation is no longer hypothetical — four frame-bounded, input-free
captures (two per variant) compare `pass`, with no unstable and no failing row — but the claim
stays `surface`, because no capture reaches a site only the release can produce.
[deterministic-capture.md](notes/deterministic-capture.md) records why that is now a scenario
problem rather than a tooling one.

![Overview](docs/progress.svg)

![Exit criteria](docs/progress-criteria.svg)

![Research surface](docs/progress-indexes.svg)

![Recent work](docs/progress-activity.svg)

| Image | Shows |
| ----- | ----- |
| `progress.svg` | Overall exit-criteria fraction, decompilation, and each milestone's fraction |
| `progress-criteria.svg` | Every exit criterion, ticked or not — the honest measure of M0–M5 |
| `progress-indexes.svg` | The searchable research surface: ten indexes by row count |
| `progress-activity.svg` | The most recent commits, so a turn's work is visible at a glance |

`scripts/make_progress_svg.py` renders all four from `docs/ROADMAP.md`, the live indexes,
`recovered/decompile.log` and `git log`, so they cannot drift from the tracker. They are
refreshed by `watchdog.ps1 -Action Start`; `--check` fails if any is stale, and
`test_progress_svg.py` asserts no text leaves the canvas and that every criterion gets
exactly one checkbox.

Each image stamps the commit it was rendered from in its bottom-right corner, which is why
an image committed alongside a change names that change's parent: the stamp records the tree
the picture was drawn from, and a commit cannot contain its own hash. The stamp is the
staleness marker — if it lags further behind than that one commit, the image is older than
the tracker and `--check` says so.

## Baseline

All work targets one pinned triple. Do not mix baselines.

| Component | Static reference | Runtime validation |
| --- | --- | --- |
| Mega Hack | v9.1.3 (2026-04-17) | same |
| Geometry Dash | 2.2081 (Windows + Android) | 2.2081, `GeometryDash.exe` `FC5A16C2…` |
| Geode | 5.6.1 | **5.11.2**, profile `runtime511` |
| Geode dependency | `geode.node-ids >= v1.22.0` | same |

`mod.json` declares `"early-load": true`, so Mega Hack initialises before other mods. A pass
under 5.11.2 is evidence about 5.11.2 and is never quoted as a 5.6.1 result — see
[notes/runtime-validation.md](notes/runtime-validation.md).

Ground truth is `samples/absolllute.hackmega/absolllute.hackmega_original.dll`
(SHA-256 `1635c339…e1107a92`, base `0x180000000`, 18,825 functions). Experimental builds in
that folder are never evidence.

## Quick start

```powershell
$py = "C:\Users\Robert_\AppData\Local\Python\bin\python3.exe"
& $py scripts\env_check.py                 # readiness: interpreter, sample digest, indexes, tools
& $py scripts\check_doc_links.py           # docs hygiene
& $py scripts\hook_sites.py --method resetLevel
& $py scripts\make_progress_svg.py --check
```

Decompiled pseudocode is optional and takes ~290 s once:

```powershell
& $py scripts\decompile_all.py --keep-project --reuse-project
```

Nothing tracked requires `samples/`; without it the sample-side tools and the two patch
validators are simply unavailable.

## PE layout (`_original.dll`)

Image base `0x180000000`, verified with `pe_caves.py`; the full table with virtual and raw
sizes is in [provenance.md](notes/provenance.md).

| Section | VA | Virtual size |
| --- | --- | --- |
| `.text` | `0x180001000` | `0x532fe6` |
| `.rdata` | `0x180534000` | `0x1aaca4` |
| `.data` | `0x1806df000` | `0x13cd90` |
| `.pdata` | `0x18081c000` | `0x39aec` |
| `.tls`, `.rsrc`, `.reloc` | `0x180856000`+ | small |

Two facts the patching work depends on:

- **Code cave** — a contiguous `0xCC` region at VA `0x180286371`, file offset `0x285771`,
  size `0xc8f`. All injected code lives there.
- **Mutable state** — 48 zero-initialised bytes at preferred VA `0x18081BFC0`, in the
  writable tail page of `.data`. Never place mutable state at an absolute address in
  `.text`; see the ASLR rule in [AGENTS.md](AGENTS.md#writing-code).

`.pdata` is also the host-side verification tool: 14,937 `RUNTIME_FUNCTION` entries in
GD 2.2081 make "is this RVA a function entry?" a set-membership test rather than a guess.

## Layout

```
AGENTS.md              working rules (evidence gates, environment, commits)
docs/GOALS.md          mission, scope, non-negotiables, definition of done
docs/ROADMAP.md        milestone tracker: criteria and status only
harness/               runtime observer (megahack.trace) and its host-side tests
notes/                 durable findings, one file per subsystem
reconstruction/        original C++ of the recovered core, host-testable
references/            external docs (Geode, GD, formats), cited and dated
samples/               GITIGNORED: the release, its tooling, vendored deps
scripts/               original tooling that outlives one investigation
```

## Findings worth knowing

Each of these is established in a note; read the note before repeating the search.

- **Mega Hack imports zero GD gameplay symbols.** All 1,010 imports over 30 DLLs are Geode,
  Windows or CRT; the host is reached by string name and by hard-coded RVAs. →
  [host-abi](notes/host-abi.md), [host-methods](notes/host-methods.md)
- **The hook surface is 338 registrations over 215 GD methods** — 164 registrar functions,
  143 registration helpers, 338 callbacks, 277 wrapper thunks, and 308 hard-coded host RVAs
  (188 distinct) plus 30 name-resolved sites. → [hook-dispatch](notes/hook-dispatch.md)
- **183 of those 188 RVAs are exact function boundaries in GD 2.2081's `.pdata`**, 145 with a
  second angle from a thunk addend; 62 callbacks are not functions under either engine and
  were classified by shape. → [hook-target-verification](notes/hook-target-verification.md)
- **Mega Hack stores no Best Run data.** The only storage-side hook is
  `GJGameLevel::savePercentage`, which can veto a save but never write one; the
  `best_runs.bin` sidecar in `samples/` is this repo's own patch, not the release. →
  [persistence](notes/persistence.md)
- **The settings registry's key list is `hackpro.json`, not the binary.** Tags are 64-bit
  `hackpro::Tag` values; `tagFor`/`keyFor` are the name↔tag join; 184 tags are read through
  six typed accessors. → [settings-registry](notes/settings-registry.md),
  [settings-contents](notes/settings-contents.md)
- **No precomputed FNV table exists**, so a guessed string key cannot be confirmed by
  finding its hash already stored. → [fnv-hash-probe](notes/fnv-hash-probe.md)
- **Searching Ghidra's `DAT_` symbols finds 4% of referenced strings.** Inlined literals and
  MSVC SSO buffers carry the rest, which is why the index has 3,609 VA-anchored and 566
  stack-built rows. → [string-inventory](notes/string-inventory.md), [sso-decoder](notes/sso-decoder.md)
- **11,003 of 18,825 functions have no direct caller**, and 280 of 281 load-time
  constructors have none. Absence of a caller is never evidence of dead code here. →
  [init-table](notes/init-table.md)
- **`_callers.tsv` omits zero-caller functions**, so a missing row means zero — reading it
  as "unknown" hides the effect. → [init-table](notes/init-table.md)
- **Mega Hack's RTTI recovers 497 classes and 794 inheritance edges**, and `numBases` sits
  at `+8`, not `+0` as MSVC documents. The Android builds carry no RTTI at all. →
  [rtti-classes](notes/rtti-classes.md), [android-abi](notes/android-abi.md)

Two failure modes are pinned as tests because they cost real time: converting an RVA to a
file offset by subtraction (both wrong reads are in `test_pe_rva.py`) and reading struct
offsets without a common base (which produced a phantom "second registry"). See
[provenance](notes/provenance.md#failure-modes-recorded) and
[settings-registry](notes/settings-registry.md).

## Tooling

`scripts/` is original work and exists on a fresh clone. The full list is `ls scripts/`;
grouped by job:

| Job | Tools |
| --- | --- |
| Read the binary | `pe_rva.py` (the only RVA→offset converter), `import_names.py`, `func_bounds.py`, `triage_detours.py` |
| Decompile and export | `decompile_all.py`, `ghidra_scripts/ExportAllFunctions.java` |
| Find code | `build_xref_index.py`, `build_callgraph.py`, `classify_strings.py`, `decode_sso.py`, `fnv_hash_probe.py`, `rtti_index.py`, `init_table.py` |
| Host ABI and hooks | `host_methods.py`, `hook_sites.py`, `resolve_wrappers.py`, `lazy_stubs.py`, `tag_callback_map.py`, `callback_hooks.py` |
| Settings | `settings_index.py` |
| Platform parity | `compare_platform_classes.py`, `verify_platform_keys.py` |
| Evidence gates | `check_inventory_evidence.py`, `check_doc_links.py` |
| Progress and ops | `make_progress_svg.py`, `env_check.py`, `notify.ps1`, `watchdog.ps1`, `run_supervised.ps1` |
| Tests | `test_pe_rva.py`, `test_import_names.py`, `test_func_bounds.py`, `test_settings_index.py`, `test_init_table.py`, `test_demangle.py`, `test_dump_tabs.py`, `test_check_doc_links.py`, `test_progress_svg.py`, `test_reconstruction.py` |

Ten indexes make the binary searchable. All live in `recovered/functions/` (gitignored) and
are regenerated by the scripts above:

| Index | Rows | Answers |
| ----- | ---: | ------- |
| `_wrappers.tsv` | 277 | which function does this thunk actually call? |
| `_xrefs.tsv` | 4,178 | which functions name this string? |
| `_settings.tsv` | 188 | which tags are read, through which accessor? |
| `_calls.tsv` / `_callers.tsv` | 42,701 | who calls what? |
| `_rtti.tsv` | 497 | which classes exist, and where is each vtable? |
| `_rtti_hierarchy.tsv` | 794 | what does each class derive from? |
| `_strings.tsv` | 4,175 | what kind of string is this, and where does it live? |
| `_init_table.tsv` | 283 | which functions run at load with no direct caller? |
| `_host_methods.tsv` | 214 | which GD methods are reached by name, and from where? |
| `_hook_sites.tsv` | 338 | which hooks are installed, on which method, at which target? |

`_hook_sites.tsv` and `_host_methods.tsv` are verified against a live Ghidra inventory;
`_host_methods.tsv` is a verified *subset* — ten hooked method names, two of them
name-resolved, are not string objects Ghidra defined, so a resolver must not treat the index
as the universe. → [hook-target-verification](notes/hook-target-verification.md)

## Trace harness

`harness/` is the host-testable core of [runtime-validation.md](notes/runtime-validation.md):
a dependency-free C++20 library that writes, reads and compares `mh-trace/1` traces,
validates `mh-scenario/1` manifests, and applies the comparability gate and comparison
rules. No Geode headers, no Windows API, no third-party dependency — it builds and tests on a
fresh clone with CMake and a compiler.

```powershell
cmake -S harness -B harness/build -G Ninja
cmake --build harness/build
ctest --test-dir harness/build --output-on-failure     # 6 entries, 194 cases
```

The fixtures are the contract's own worked examples, extracted from the note by
`harness/tools/gen_example_*_fixture.py` so they cannot drift from it: the seven-line trace
must validate and round-trip byte for byte, and the example manifest must validate while
staying `draft` and therefore ineligible for evidence. The synthetic corpus produces one
comparison pair per result status (`pass`, `pass_with_warnings`, `fail`, `blocked`,
`unstable`, `invalid`). The record shapes the observer emits are built by
`harness/include/mh/records.hpp` and pinned byte-for-byte against a captured trace, so a mod
cannot drift from the contract without a test failing.

Proving the contract is executable is not runtime validation. The Geode observer
(`megahack.trace`) and the second comparison target (`megahack.reconstruction`) both build and
package. The level fixture exists and is generated, not hand-made:
`harness/tools/make_level_fixture.py` writes `harness/fixtures/runtime/safe-mode-smoke.gmd`
(947 bytes, `810e276e…`) from a declarative spec, and installs it only through the sandbox. The
manifest's `level.sha256` is no longer a placeholder: the observer records the SHA-256 of the
level string at `level_started` (`58d4b320…` for the fixture, the encoded `k4` form the game
holds) and the gate compares each run's observation with it — `blocked` when a run recorded
none, `invalid` when a run started a different level. Four captures (two per variant) have now been taken, and they compare
`pass`; the claim they support is `surface`, and
[deterministic-capture.md](notes/deterministic-capture.md) is the record.
Method, evidence and open questions: [trace-harness.md](notes/trace-harness.md),
[trace-observer.md](notes/trace-observer.md), [reconstruction-variant.md](notes/reconstruction-variant.md),
[gd-save-format.md](notes/gd-save-format.md); running logs:
[harness/PROGRESS.md](harness/PROGRESS.md), [harness/OBSERVER-PROGRESS.md](harness/OBSERVER-PROGRESS.md).

The runner half lives in `harness/tools/`: `preflight.py` (hashes, profile guard, run context,
settings tags), `grade_trace.py` (grade the newest capture), `gd_save.py` (the 2.2081 save and
`k4` container), `make_level_fixture.py` (the generated level), `save_sandbox.py` (guarded,
atomic, digest-verified backup/install/restore of a save directory), `verify_record_bytes.py`
(record shapes, plus a player-data source guard), and `test_compare_rehearsal.py`, which drives
the real comparison CLI over four synthetic traces so the gate's integration is exercised
before anyone spends four real runs. All of them test on temporary directories only.

## Notes map

`notes/` holds 84 files and no index beyond this one. Start with the subsystem, not the
feature.

| Theme | Notes |
| --- | --- |
| Provenance and audits | `provenance`, `m1-audit`, `m1-verification`, `m1-residuals`, `address-map` |
| Host ABI | `host-abi`, `host-methods`, `gd-structures`, `hook-dispatch`, `hook-target-verification`, `hook-wrapper-layer`, `hook-registrations` |
| Settings and API | `settings-registry`, `settings-contents`, `hackpro-api`, `definition-keys`, `tab-registration`, `nxui-menu`, `entry-path` |
| Binary inventory | `string-inventory`, `sso-decoder`, `rtti-classes`, `init-table`, `import-dt`, `static-hack-label-map` |
| Hooks by domain | `input-hooks`, `render-hooks`, `gameplay-hooks`, `ui-addons`, `save-data-hooks` |
| Persistence | `persistence`, `save-lifecycle`, `import-dt`, `gd-save-format` |
| Features | one `<feature>-feature.md` per mapped feature, plus `unlock-bypasses` |
| Android parity | `android-abi`, `android-symbol-parity`, `android-string-parity`, `android-input-parity` |
| Validation and state | `feature-inventory`, `m3-validation`, `runtime-validation`, `runtime-validation-handoff`, `trace-harness`, `trace-observer`, `deterministic-capture`, `reconstruction-variant`, `geode-skeleton`, `portable-build` |

## Sample tooling

Everything under `samples/` is untracked, so these exist only once the sample set is
restored. Every tool does `sys.path.insert(0, ".analysis_tools")`, so **run from the mod
root**:

```powershell
Set-Location samples/absolllute.hackmega
& $py .\tools\pe_disasm.py .\absolllute.hackmega_original.dll 0x180199e80
```

| Tool | Purpose |
| --- | --- |
| `pe_disasm.py` | disassemble at a VA/RVA; `--callers` finds direct call sites |
| `pe_xrefs.py` | locate strings (ASCII/UTF-16) and the code that references them |
| `pe_caves.py` | sections plus uniform-byte caves in executable sections |
| `pe_exports.py`, `pe_imports.py` | exports and IAT surface |
| `build_persistence_patch.py`, `validate_persistence_patch.py`, `best_runs_db.py` | the local patch experiment and its gates |

Ghidra 12.1.2 lives at `C:\Users\Robert_\Downloads\ghidra_12.1.2_PUBLIC_20260605`; its
`HackMega` scripts (`DumpDecompile`, `DumpDecompileFile`, `FindAsciiReferences`) are in
`samples/absolllute.hackmega/tools/ghidra_scripts/`. Compile them with `javac` against the
install's jars first — a Ghidra script compile error surfaces as `ClassNotFoundException`.

The REA MCP server is the second engine (Ghidra and Rizin providers). One import costs about
six minutes, so batch whole-program questions in one session and close it with a snapshot;
see [AGENTS.md](AGENTS.md#environment).

## Workflow

1. **Pin the question.** One investigation, one written question, one note.
2. **Locate.** String xrefs, call graph, host-method index — in that order.
3. **Disassemble.** Raw bytes decide; the decompiler only suggests.
4. **Prove.** Re-derive from a second, genuinely different angle.
5. **Record.** Note with question, method, evidence, conclusion, open questions.
6. **Only then** write code, in `reconstruction/` or `harness/`, with tests.

Three patch-site labels in `0x1801552xx` are corroborated by Mega Hack's own name resolution;
the rest remain the local patch's labels. The distinction, and the addresses, are in
[host-methods.md](notes/host-methods.md#corroborating-two-of-the-patchs-hook-site-labels).

## Disclaimer and license

For interoperability, research, preservation and compatibility. No copyrighted software,
firmware, keys or proprietary libraries are included or required. Mega Hack is the work of
Absolllute; Geometry Dash is the work of RobTop Games; Geode is a separate project. Anyone
using binaries here is responsible for holding the relevant rights and complying with
applicable law and licence terms.

Original tooling and notes are licensed under the [GNU General Public License v3](LICENSE).
Those third-party works are not relicensed by this repository.
