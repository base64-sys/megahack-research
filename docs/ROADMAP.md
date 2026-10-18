# Roadmap

Baseline: **Mega Hack v9.1.3 / Geometry Dash 2.2081 / Geode 5.6.1**, image base
`0x180000000`, primary target `absolllute.hackmega_original.dll`.

A milestone is `done` only when its exit criteria are met. Scope changes go in
[GOALS.md](GOALS.md), not here.

## Milestones

| # | Milestone                              | Status      | Depends on |
| - | -------------------------------------- | ----------- | ---------- |
| M0 | Environment, provenance and tooling    | done        | —          |
| M1 | Binary inventory and host ABI surface  | in progress | M0         |
| M2 | Core infrastructure reconstruction     | not started | M1         |
| M3 | Feature module mapping                | not started | M2         |
| M4 | Sidecar format and importer parity     | not started | M2         |
| M5 | Android ARM64 parity                   | not started | M4         |

### M0 — Environment, provenance and tooling (done)

Establish that the baseline is identifiable, that the tooling runs, and that nothing
proprietary is version-controlled.

Exit criteria — all met:

- [x] Baseline pinned from `mod.json` and `changelog.md`: v9.1.3, GD 2.2081, Geode 5.6.1,
      `geode.node-ids >= v1.22.0`, `early-load: true`.
- [x] Ground-truth binary distinguished from experiments:
      `_original.dll` is the read-only reference; `_pre_persistence`, `_working_aslr` and
      the generated `.dll` are experiments and never used as evidence.
- [x] Section map, image base, code cave and mutable-state page verified with
      `pe_caves.py`.
- [x] Toolchain located and working: Python 3.14.6, capstone 5.0.9, keystone-engine
      0.9.2, pefile 2024.8.26, Ghidra 12.1.2 with three `HackMega` headless scripts.
- [x] `samples/` untracked; `.gitignore` blocks `*.dll`, `*.exe`, `samples/`, `decompiled/`.
- [x] Repo skeleton and rules written down.

Remaining risk: the sample set cannot be checksum-pinned by tracking the binaries, so the
digests live in `notes/provenance.md` instead and must be re-checked by hand whenever the
sample set is refreshed. M1 found a sharper version of this risk — the `.geode` package in
`samples/` is already modified, so only `_original.dll` is pristine.

### M1 — Binary inventory and host ABI surface (in progress)

Turn the binary into a navigable map before reading any logic. The payoff is that every
later milestone gets cheap lookups instead of fresh searching.

Exit criteria:

- [x] SHA-256 of each baseline binary recorded in `notes/provenance.md`, and **cross-checked
      against a pristine v9.1.3 package**. `samples/PRISTINE_MEGAHACK.geode` was supplied on
      2026-10-17; its `absolllute.hackmega.dll` is bit-identical to
      `absolllute.hackmega_original.dll` (`1635c339…`), as are both Android `.so` files. The
      baseline is now independently verified — see
      [provenance.md](../notes/provenance.md#cross-check-against-a-pristine-package).
- [x] Export table recovered: 30 named MSVC-mangled symbols, demangled with
      `undname.exe`, recorded in `notes/hackpro-api.md`. The export table is **not** stripped.
- [x] Full import table with preferred-base IAT addresses classified into Geometry Dash,
      Geode, Windows and CRT — `notes/host-abi.md`. 1,010 imports over 32 DLLs; all 646
      mangled symbols demangled. Headline: **zero** GD gameplay classes are statically
      imported; they are reached by runtime name resolution.
- [ ] Full string inventory split into user-visible labels, setting keys, file paths,
      language keys and format strings, each anchored to a VA.
- [x] Mega Hack's own RTTI/vtable set enumerated; the class hierarchy recovered. Partly
      pre-solved: namespaces `hackpro`, `hackpro::extensions` and `mega::utils::safe_mode`
      are known from the export table rather than inferred. **Completed by
      `scripts/rtti_index.py`**: 517 type descriptors, 497 Complete Object Locators validated
      by `signature == 1` *and* `pSelf` pointing at themselves, and 497 vtables resolved.
      Namespaces recovered are `hackpro`, `core`, `mega::extensions`, `mega::utils`, `iconic`
      and `tabcore` — the last three were not previously on the map. 794 distinct inheritance
      edges in `_rtti_hierarchy.tsv`. Two structural corrections are recorded in
      [rtti-classes.md](../notes/rtti-classes.md): `numBases` sits at `+8` in the hierarchy
      descriptor, not `+0` as MSVC documents, and `undname.exe` cannot parse these type
      descriptors at all. Caveat: template argument scoping is not tracked in the demangler,
      so a minority of base names are approximations — the `mangled` column is authoritative.
- [ ] Settings registry map: every key, its type, default, and the function that applies it.
      **Partly done.** The registry class, its `+0x128` `Config` base layout, the 64-bit
      `hackpro::Tag` key type, the tag→name map, `keyFor` (`0x180072E20`), the by-tag
      setter (`0x18008B490`) and the five value types are now mapped in
      [settings-registry.md](../notes/settings-registry.md). Still missing: the insertion
      site where a name becomes a tag, so **defaults and the complete key list are not
      recovered** - which is why the criterion stays open.
- [ ] Save/load file formats mapped, including the sidecar `best_runs.bin` header and
      record layout.
- [x] Entry and lifecycle order, partially: `geodeImplicitEntry` at `0x180072B60` is the
      root, and the load-time sequence registering Mega Hack's two tabs plus resolving the
      `Import .dt` action tag is mapped in `notes/tab-registration.md`. TLS callbacks,
      `early-load` ordering and the shutdown path still outstanding.
- [ ] Geometry Dash structures Mega Hack touches documented with verified offsets
      (`GJGameLevel+0x148`, `+0x14C`, `+0x158`, `+0x198`, `+0x1B8`, `+0x2C4`, `+0x35C`).
- [ ] Output: `notes/` map files, one per subsystem, each fact cited by VA. Four written
      (`provenance`, `hackpro-api`, `host-abi`, `tab-registration`).

Status: layout, cave, state block, patch-site set, hook map, level-identity scheme and DB
format are known and now re-derived from `_original.dll` in `notes/provenance.md`. Export
table and extension API are in `notes/hackpro-api.md`. Host ABI and the GD runtime-resolution
path in `notes/host-abi.md`. Tab registration in `notes/tab-registration.md`. Still
outstanding: the string inventory, full RTTI, settings registry, remaining lifecycle, and
the `GJGameLevel` offsets.

The runtime GD resolution path identified in `notes/host-abi.md` is the highest-value target
left in M1, and arguably belongs to M2: every feature reaches gameplay through it.

### M2 — Core infrastructure reconstruction (not started)

The shared machinery every feature depends on: module entry, settings system, the `nxUI`
menu/interface layer, hook dispatch, save lifecycle.

Exit criteria:

- [ ] Module entry and `early-load` path reconstructed as compiling C++. Root is
      `geodeImplicitEntry` at `0x180072B60`, confirmed by disassembly — see
      [hackpro-api.md](../notes/hackpro-api.md).
- [ ] Settings registry reconstructed: typed keys, defaults, load/save, and the boolean
      setter that Mega Hack uses to intercept settings by tag. The exported shape is a
      string-keyed, typed config with five value types (`bool`, `double`, `int64`,
      `std::string`, `std::vector<unsigned int>`), interned `Tag` identity, and per-key typed
      change listeners — see [hackpro-api.md](../notes/hackpro-api.md#what-the-signatures-imply).
- [ ] `nxUI` menu reconstructed well enough to explain how rows, tags and languages
      interact — including why a packed interface has no spare row and a definition hook
      substitutes a label per language dictionary load.
- [ ] Hook dispatch mechanism recovered, so new hooks are mechanical rather than bespoke.
- [ ] Save lifecycle reconstructed: `postUpdate`, `resetLevel`, `PlayLayer::onQuit` and
      shutdown-destructor flush points.
- [ ] A minimal Geode-compatible skeleton loads under the loader with core behaviour intact.

### M3 — Feature module mapping (not started)

Mega Hack ships 160+ toggles. Map each to the code that implements it.

Exit criteria:

- [ ] Every feature listed in `changelog.md` accounted for: implemented, alias, or dead.
- [ ] Each feature mapped to its implementing function(s) with VAs, its settings keys, its
      default, and its interactions.
- [ ] High-value clusters documented as patterns rather than 160 isolated notes: input
      hooks, render hooks, gameplay rewrites, UI add-ons, save-data features.
- [ ] Reconstructed modules validated against release behaviour for the sampled clusters.

### M4 — Sidecar format and importer parity (not started)

Own the `best_runs.bin` format and the Death Tracker import path end to end.

Exit criteria:

- [ ] Format spec complete: 32-byte header, 24-byte records, `FNV1aChecksumOfRecords`, and
      every rejection rule (magic/version/size, >65,536 records, truncation, trailing data,
      checksum, zero key, NaN/infinite, `0 <= start <= end <= 100`).
- [ ] Level-key scheme documented as three non-overlapping namespaces: decoded `m_levelID`
      for online levels, stable editor ID with bit 62 for local levels, FNV-1a content hash
      with bit 63 as fallback.
- [ ] Ambiguous legacy records are skipped rather than mis-attributed.
- [ ] Atomic write lifecycle reproduced: `.tmp` → rotate → install, `.bak` rollback rules,
      and the recovery flag that protects a known-good backup.
- [ ] Native `Import .dt` action reproduced end to end: `GetOpenFileW` wrapper, flush, import,
      reload, cancel-is-a-no-op.
- [ ] Validation suite green: `validate_persistence_patch.py` plus the 12 DB/import tests.

### M5 — Android ARM64 parity (not started)

Deferred by design. Begins only when M4 is `done`.

Exit criteria:

- [ ] ARM64 `lib` mapped against the x86-64 map; divergences catalogued rather than assumed.
- [ ] Behaviour of the same features confirmed on device.
- [ ] Cross-platform implementation compiles from one source tree.

## Cross-cutting work

These are tracked per milestone rather than as milestones of their own.

| Item                                   | Where | State   |
| -------------------------------------- | ------ | ------- |
| SHA-256 baseline pinning                | M1     | done, cross-check passed |
| Address map under version control       | M1     | open    |
| `notes/` entries from RE_NOTES promoted out of `samples/` | M1 | in progress |
| Script promotion policy (`scripts/` vs scratch) | all | defined in AGENTS.md |
| Automated re-baseline diff on version bump | all | open |
| Demangler wrapper around `undname.exe`  | all | candidate for `scripts/` |
| Decide which `samples/.../tools/` are original and may be tracked | M1 | open |
| Safe RVA→file mapping helper            | M1     | candidate for `scripts/` |

## Deferred, deliberately

- Shader or render-pipeline reconstruction.
- Unshipped or dead code — mapped as dead, not recovered.
- Windows-only third-party anti-cheat interaction, unless a feature provably depends on it.