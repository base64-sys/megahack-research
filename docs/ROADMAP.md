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
| M4 | Persistence and the `.dt` importer      | not started | M2         |
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
- [x] Full string inventory split into user-visible labels, setting keys, file paths,
      language keys and format strings, each anchored to a VA. 17 categories over 4,175
      strings (3,609 VA-anchored, 566 SSO); 957 OpenSSL names reclassified to
      `third-party` by code-region evidence (boundary 0x1803A4EE0). Residual 353 rows
      (8.5%) are JSON keywords, CRT constants and SSO decoder artefacts, characterised in
      [string-inventory.md](../notes/string-inventory.md). Done 2026-10-21.
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
      **Mechanism mapped, contents relocated, accessor list now complete (six accessors).**
      The registry
      class, its `+0x128` `Config` base layout, the 64-bit `hackpro::Tag` key type, the
      tag↔name bijection, `tagFor` (`0x180072DF0`), `keyFor` (`0x180072E20`), the by-tag
      setter (`0x18008B490`) and the five value types are all mapped in
      [settings-registry.md](../notes/settings-registry.md). The full key list is
      **not recoverable from the binary**: it is `hackpro.json` in the installed mod
      folder, loaded by `FUN_18008B780` and interned by `FUN_18007C700`, whose schema is
      now documented. That is also the answer to the string-inventory gap — the join
      between UI widget names and numeric tags is `tagFor` at the exported API boundary.
      Outstanding: positional hook-target pairings are now derived from the registration text
      for all 338 sites; 30 of them are name-resolved rather than RVA-based. The RVAs flagged
      UNVERIFIED per row in `_hook_sites.tsv` are those not yet checked against a thunk. The registry object is
      now identified — the global at `0x180800F58`, at least `0x4A8` bytes, in `.data`'s
      zero-fill tail with no file backing, and every accessor receives it `+0xD8`. Its
      constructor writing `hackpro::Config::vftable` at `+0x00` and `+0x128` independently
      confirms the `Config`-at-`+0x128` layout. The apparent "`+0xB0` is claimed twice"
      conflict is **resolved**: the two readers use different bases — the serialiser arrives
      by virtual dispatch at `+0x128`, the accessors at `+0xD8` — so they are different
      fields, and disassembly settles the serialiser's as a circular list head
      (`mov rsi,[rdi+0xb0]`, then `r13 = *r13` until `r13 == rsi`). The field grouping of
      the `Bytes` map remains UNVERIFIED and needs one function read. The criterion stays open
      because the *contents* are data, and that data is user data this repo deliberately
      does not commit.
- [ ] Save/load file formats mapped. **The three stores the release uses are now
      separated**, which was ambiguous before: `hackpro.json` in the mod directory (read
      `0x1800BF190`, written `0x1800BF400`, both dispatched indirectly so both have zero
      direct callers) and GD's `data.json` / `save.json` under `%LOCALAPPDATA%\…\Octavus\`
      (read by `0x180042FB0`; `%LOCALAPPDATA%` confirmed from the `FOLDERID` GUID at
      `0x1805820E0`). **Correction:** this criterion previously named "the sidecar
      `best_runs.bin` header and record layout", which is the local patch's format and not
      the release's — see [persistence.md](../notes/persistence.md). The path segment and
      the native Best Run field mapping are still outstanding.
- [x] Entry and lifecycle order, partially: `geodeImplicitEntry` at `0x180072B60` is the
      root, and the load-time sequence registering Mega Hack's two tabs plus resolving the
      `Import .dt` action tag is mapped in `notes/tab-registration.md`. TLS callbacks,
      `early-load` ordering and the shutdown path still outstanding.
- [ ] Geometry Dash structures Mega Hack touches documented with verified offsets
      (`GJGameLevel+0x148`, `+0x14C`, `+0x158`, `+0x198`, `+0x1B8`, `+0x2C4`, `+0x35C`).
      **Partially verified 2026-10-21.** Five of the seven are now confirmed in
      `_original.dll` from two independent angles each: `+0x148`, `+0x14C`, `+0x158`,
      `+0x198`, `+0x1B8` (see [gd-structures.md](../notes/gd-structures.md)).
      `+0x2C4` and `+0x35C` are not accessed on a GJGameLevel in this binary and remain
      UNVERIFIED. Mechanism recovered as before: `scripts/host_methods.py` indexes the 214
      method-name strings, and Mega Hack calls GD through absolute RVAs into
      `GetModuleHandleA(NULL)` (see [hook-dispatch.md](../notes/hook-dispatch.md)).
      Three GD function RVAs and `PlayLayer+0x3230` are verified from two angles each.
      Mega Hack does **not** use `node-ids` for this, so verified offsets change on any
      GD re-baseline.
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
      **Recovered.** Mega Hack takes `GetModuleHandleA(NULL)` — the GD executable's base,
      `FUN_18010DDC0`, 278 direct callers — and treats hard-coded RVAs into it as GD
      function addresses, wrapping each in a Geode hook under a convention its debug types
      name `TulipConvention`. Registering is
      `(resolver, targetFromGDBase(rva), callback, name, -32, 1)`; calling an original is
      `createWrapper(GetModuleHandleA(NULL) + rva)`. Three GD function RVAs and one GD
      field offset (`PlayLayer+0x3230`) are verified from two angles each. See
      [hook-dispatch.md](../notes/hook-dispatch.md). **The surface is now enumerated** by
      `scripts/hook_sites.py`: 338 registrations over 215 distinct GD methods, each method
      mapping to exactly one host target (RVA or name-resolved), and four of those RVAs
      confirmed from two angles.
      A reconstruction should reproduce the mechanism, not copy the RVAs — they are
      absolute into the host image and change on any GD re-baseline.
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

### M4 — Mega Hack persistence and the `.dt` importer (not started)

Map what the release binary actually stores and how it loads it, plus the native
Death Tracker import path.

> **Corrected 2026-10-18.** This milestone previously read "Own the `best_runs.bin` format
> and the Death Tracker import path end to end", with exit criteria for a 32-byte header,
> `MHBRDB1` magic and 24-byte records. **That format is not Mega Hack's** — it is the local
> persistence patch's own invention. A byte scan of all four Windows images finds
> `best_runs`/`MHBRDB1` only in binaries produced by `build_persistence_patch.py`, and
> `_xrefs.tsv` gives them zero referencing functions. See
> [persistence.md](../notes/persistence.md). The sidecar criteria have been removed rather
> than kept as aspirational, because implementing them would build a format the release
> never had. The sample patch's own tooling still lives in `samples/` and stays out of
> scope for the reconstruction.

Exit criteria:

- [ ] Mega Hack's native Best Run storage located and documented: which of GD's `data.json`
      fields carry `(level, startPercent) -> bestEndPercent`, and on which flush points it
      is written.
      **Answered, negatively: Mega Hack stores no Best Run data.** Sweeping every hook
      registration site yields 338 sites over 215 distinct GD methods. The only
      storage-side hook is `GJGameLevel::savePercentage`, whose
      callback can only *veto* the call, never write. Best Run is GD's own saved percentage,
      and Mega Hack's role is to stop a cheated run from being persisted. See
      [persistence.md](../notes/persistence.md). Residual work closed 2026-10-21:
      the `(level, startPercent) -> bestEndPercent` fields are GD's schema inside
      `GeometryDash.exe` and are **not recoverable from Mega Hack's binary** — Mega Hack
      never reads or writes them (it only vetoes `savePercentage`). Marked UNVERIFIED in
      [persistence.md](../notes/persistence.md#native-best-run-fields-gds-schema-not-mega-hacks).
- [ ] `hackpro.json` read/write lifecycle mapped: which of `FUN_1800BF190` /
      `FUN_1800BF400` are called from where, given both have zero direct callers.
- [ ] The `%LOCALAPPDATA%` → `Octavus` path segment resolved.
- [ ] Native `Import .dt` action reproduced end to end: `GetOpenFileNameW` wrapper,
      importer dispatch at `0x180027C30`, tag resolution at `0x1800023DC`, label
      substitution at `0x180098868`, cancel-is-a-no-op.
- [ ] Reconstructed persistence code validated against release behaviour on the sampled
      stores.

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