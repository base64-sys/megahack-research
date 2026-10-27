# Roadmap

Baseline: **Mega Hack v9.1.3 / Geometry Dash 2.2081 / Geode 5.6.1**, image base
`0x180000000`, primary target `absolllute.hackmega_original.dll`. Runtime validation uses
Geode **5.11.2** in the isolated `runtime511` profile; that is a separate baseline, not a
new static reference ([runtime-validation.md](../notes/runtime-validation.md)).

A milestone is `done` only when its exit criteria are met. This file holds criteria and
status; the evidence lives in `notes/`. Scope changes go in [GOALS.md](GOALS.md).

## Milestones

| # | Milestone | Status | Depends on | Outcome |
| - | --------- | ------ | ---------- | ------- |
| M0 | Environment, provenance and tooling | done | — | the baseline is identifiable and the tooling works |
| M1 | Binary inventory and host ABI surface | done | M0 | every lookup is cheap instead of a fresh search |
| M2 | Core infrastructure reconstruction | done | M1 | entry, settings, UI, hook dispatch and save lifecycle in C++ |
| M3 | Feature module mapping | in progress | M2 | every feature mapped to its code, and validated at runtime |
| M4 | Persistence and the `.dt` importer | done | M2 | what the release stores, and what it does not |
| M5 | Android ARM64 parity | in progress | M4 | the same features, confirmed on device |

### M0 — Environment, provenance and tooling (done)

Objective: prove the baseline is identifiable, the tooling runs, and nothing proprietary is
version-controlled.

- [x] Baseline pinned from `mod.json` and `changelog.md`: v9.1.3, GD 2.2081, Geode 5.6.1,
      `geode.node-ids >= v1.22.0`, `early-load: true`.
- [x] Ground truth distinguished from experiments: `_original.dll` is read-only reference;
      `_pre_persistence`, `_working_aslr` and generated `.dll`s are never evidence.
- [x] Section map, image base, code cave and mutable-state page verified with `pe_caves.py`
      (`README.md#pe-layout`).
- [x] Toolchain located and working: Python 3.14.6, capstone, keystone, pefile, Ghidra
      12.1.2 with headless scripts, and `scripts/env_check.py` to re-check it in one pass.
- [x] `samples/` untracked; `.gitignore` blocks binaries, assets, `recovered/` and
      `runtime-traces/`.
- [x] Repo skeleton and rules written down ([AGENTS.md](../AGENTS.md), [GOALS.md](GOALS.md)).

Residual risk: the sample set cannot be checksum-pinned by tracking it, so digests live in
[provenance.md](../notes/provenance.md) and must be re-checked when the samples are
refreshed. The `.geode` package in `samples/` is already modified — only `_original.dll` is
pristine.

### M1 — Binary inventory and host ABI surface (done)

Objective: turn the binary into a navigable map so later milestones get lookups instead of
searches. Detail and audits: [m1-audit.md](../notes/m1-audit.md),
[m1-verification.md](../notes/m1-verification.md),
[m1-residuals.md](../notes/m1-residuals.md), [address-map.md](../notes/address-map.md).

- [x] SHA-256 of each baseline binary in [provenance.md](../notes/provenance.md),
      cross-checked against a pristine v9.1.3 package: `absolllute.hackmega.dll` is
      bit-identical to `_original.dll` (`1635c339…`), as are both Android `.so` files.
- [x] Export table recovered: 30 named MSVC-mangled symbols, demangled, in
      [hackpro-api.md](../notes/hackpro-api.md). The table is **not** stripped — the binary
      has no debug info and few exports, not no exports.
- [x] Full import table: 1,010 imports over 30 DLLs, classified into GD, Geode, Windows and
      CRT, 646 mangled symbols demangled. Headline: **zero** GD gameplay classes are
      statically imported; [host-abi.md](../notes/host-abi.md).
- [x] String inventory: 4,175 code-referenced strings in 16 categories (3,609 VA-anchored,
      566 stack-built), with the library/Mega Hack boundary at `0x1803A4EE0` and a residual
      `other` bucket of 353 rows characterised rather than unknown;
      [string-inventory.md](../notes/string-inventory.md).
- [x] Mega Hack's RTTI/vtable set: 517 type descriptors, 497 validated Complete Object
      Locators, 497 vtables, 794 inheritance edges, namespaces `hackpro`, `core`,
      `mega::extensions`, `mega::utils`, `iconic`, `tabcore`;
      [rtti-classes.md](../notes/rtti-classes.md).
- [x] Settings registry mechanism: the registry global `0x180800F58`, `Config` at `+0x128`,
      the chained FNV-1a-64 map, six typed accessors and 184 read tags; contents (202 keys)
      from `hackpro.json` in [settings-contents.md](../notes/settings-contents.md). The
      name↔tag join is runtime (`tagFor`/`keyFor`) and cannot be closed statically.
- [x] Save/load formats: the three stores (`hackpro.json`, GD `data.json`, GD `save.json`),
      their paths and `%LOCALAPPDATA%` resolution; native Best Run field mapping recovered
      from GD 2.2081. **Correction:** the `best_runs.bin` sidecar is this repo's patch, not
      the release ([persistence.md](../notes/persistence.md)).
- [x] Entry and lifecycle order: `geodeImplicitEntry` at `0x180072B60`, early-load callbacks,
      CoreDirector virtual #20, descriptor install, `atexit(FUN_180001370)`, no TLS
      callbacks; [tab-registration.md](../notes/tab-registration.md),
      [entry-path.md](../notes/entry-path.md).
- [x] GD structures Mega Hack touches, with verified offsets: `+0x148`, `+0x14C`, `+0x158`,
      `+0x198`, `+0x1B8` (two angles each) and `PlayLayer+0x3230`. `+0x2C4`/`+0x35C` are
      **not** GJGameLevel offsets — they live on a different GD structure;
      [gd-structures.md](../notes/gd-structures.md).

- [x] One note per M1 subsystem, every address claim VA-anchored: `provenance`,
      `hackpro-api`, `host-abi`, `host-methods`, `hook-dispatch`, `hook-wrapper-layer`,
      `tab-registration`, `init-table`, `string-inventory`, `sso-decoder`,
      `settings-registry`, `rtti-classes`, `gd-structures`, `persistence`, `address-map`,
      plus the three audit notes. The map of all notes is in [README.md](../README.md#notes-map).

M1 residuals, deliberately open:

- `+0x2C4` / `+0x35C` as GJGameLevel offsets: a machine-code sweep shows they belong to
  another structure, so they stay UNVERIFIED here.
- The serialiser's `this` base (its `+0xB0` may be the accessors' `+0xB0`) and the exact C++
  type of the registry's map container.
- The release-side shutdown wrapper `0x180155155`; the patch-side wrapper is the patch's.
- Two of 188 hook-target RVAs (`0xC77D0`, `0xC7810`) have only the registration argument;
  183 verify exactly against GD 2.2081's `.pdata` and 145 carry a thunk addend as well. The
  per-row UNVERIFIED column this line used to cite never existed in `_hook_sites.tsv`; the
  measured status is in [hook-target-verification.md](../notes/hook-target-verification.md).

### M2 — Core infrastructure reconstruction (done)

Objective: the machinery every feature depends on, as compiling C++ in `reconstruction/`.

- [x] Module entry and `early-load` path reconstructed. Root is `geodeImplicitEntry` at
      `0x180072B60` ([hackpro-api.md](../notes/hackpro-api.md)).
- [x] Settings registry reconstructed: typed keys, defaults, load/save, per-key listeners,
      interned `Tag` identity, five value types ([hackpro-api.md](../notes/hackpro-api.md)).
- [x] `nxUI` menu reconstructed well enough to explain rows, tags and languages — including
      why a packed interface has no spare row and a definition hook substitutes a label per
      language load ([nxui-menu.md](../notes/nxui-menu.md), [definition-keys.md](../notes/definition-keys.md)).
- [x] Hook dispatch recovered so new hooks are mechanical: `GetModuleHandleA(NULL)` +
      hard-coded RVA, wrapped in a Geode hook under `TulipConvention`, registered as
      `(resolver, target, callback, name, -32, 1)`; calling an original is
      `createWrapper(GetModuleHandleA(NULL) + rva)` ([hook-dispatch.md](../notes/hook-dispatch.md)).
- [x] Save lifecycle reconstructed (`postUpdate`, `resetLevel`, `PlayLayer::onQuit`,
      shutdown destructor) and a minimal Geode-compatible skeleton loads with core behaviour
      intact ([save-lifecycle.md](../notes/save-lifecycle.md), [geode-skeleton.md](../notes/geode-skeleton.md)).

A reconstruction reproduces the mechanism, not the RVAs: they are absolute into the host
image and change on any GD re-baseline.

### M3 — Feature module mapping (in progress)

Objective: map every shipped toggle to the code that implements it, then prove the
reconstruction behaves like the release.

- [ ] Every feature in `changelog.md` accounted for: implemented, alias or dead.
      389 unique `Added …` wordings, **106 mapped**, 284 still `cluster`; the universe
      excludes features named only in fix bullets (`No Transition`, `Unlock Icons`,
      `Music Unlocker`) and in `New For 2.2 Mods`. Authoritative list:
      [feature-inventory.md](../notes/feature-inventory.md).
- [ ] Each feature mapped to its implementing function(s) with VAs, its settings keys, its
      default and its interactions. **125 tag registrations** over 125 distinct tags, and
      **186 same-image hooks across 111 callbacks**, are enumerated from the instruction
      bytes ([hook-registrations.md](../notes/hook-registrations.md)); seven rows have
      per-feature evidence ([static-hack-label-map.md](../notes/static-hack-label-map.md),
      [safe-mode-feature.md](../notes/safe-mode-feature.md),
      [show-hitboxes-feature.md](../notes/show-hitboxes-feature.md)). The rest keep this
      open.
- [x] High-value clusters documented as patterns, not per-feature essays: input hooks,
      render hooks, gameplay rewrites, UI add-ons, save-data features.
- [ ] Reconstructed modules validated against release behaviour for the sampled clusters.
      **Method fixed** in [runtime-validation.md](../notes/runtime-validation.md): JSONL
      trace schema, scenario manifest, comparability gate, comparison rules and the M3
      evidence checklist. The observer, runner and comparator now exist and reach a verdict
      on real captures: four bounded no-input runs (two per variant) compare `pass` with no
      unstable and no failing row, but the claim is `surface`, because the one
      target-dependent site (`0x16c8b0`/`save`) never fires inside the bounded window. All
      five clusters in [m3-validation.md](../notes/m3-validation.md) therefore remain
      structural-only until a scenario reaches behaviour; the reason is measured rather than
      assumed ([deterministic-capture.md](../notes/deterministic-capture.md), "Result").

### M4 — Persistence and the `.dt` importer (done)

Objective: what the release actually stores and how it loads it.

- [x] Native Best Run storage located and documented — **answered negatively**: Mega Hack
      stores no Best Run data. Sweeping all 338 registration sites leaves
      `GJGameLevel::savePercentage` as the only storage-side hook, and it can only veto.
      Best Run is GD's own percentage ([persistence.md](../notes/persistence.md)).
- [x] `hackpro.json` read/write lifecycle mapped: both entry points are dispatched
      indirectly, which is why both have zero direct callers.
- [x] `%LOCALAPPDATA%` → `Octavus` path segment resolved (string table `0x18053FBA5`).
- [x] Native `Import .dt` reproduced end to end: `GetOpenFileNameW` wrapper, importer
      dispatch `0x180027C30`, tag resolution `0x1800023DC`, label substitution
      `0x180098868`, cancel-is-a-no-op ([import-dt.md](../notes/import-dt.md)).
- [x] Reconstructed persistence validated against the release on the sampled stores.

The `best_runs.bin` criteria that this milestone used to carry were removed, not deferred:
that format belongs to this repo's patch, and implementing it would build something the
release never had.

### M5 — Android ARM64 parity (in progress)

Objective: re-apply the x86-64 map to the ARM64 build and confirm it on device.

- [x] ARM64 `lib` mapped against the x86-64 map, divergences catalogued rather than assumed:
      3,052 `_ZN` method symbols and zero typeinfo, so the 497-class RTTI inventory does not
      carry across ([android-abi.md](../notes/android-abi.md),
      [android-symbol-parity.md](../notes/android-symbol-parity.md)).
- [ ] Behaviour of the same features confirmed on device.
- [ ] Cross-platform implementation compiles from one source tree. Builds and passes nine
      behavioural checks under MinGW-W64 g++ 16.2.0 with no Windows API outside one
      visibility macro ([portable-build.md](../notes/portable-build.md)); an NDK/aarch64
      build is outstanding.

## Cross-cutting

| Item | Where | State |
| --- | --- | --- |
| SHA-256 baseline pinning | M1 | done, cross-check passed |
| Address map under version control | M1 | done ([address-map.md](../notes/address-map.md)) |
| Promotion of sample-side RE notes into `notes/` | M1 | in progress |
| Automated re-baseline diff on version bump | all | open |
| Decide which `samples/.../tools/` are original and may be tracked | M1 | open |
| Safe RVA→file mapping helper | M1 | done (`scripts/pe_rva.py`, tested) |

## Deferred, deliberately

- Shader or render-pipeline reconstruction.
- Unshipped or dead code — mapped as dead, not recovered.
- Windows-only anti-cheat interaction, unless a feature provably depends on it.
