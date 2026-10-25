# Roadmap

Baseline: **Mega Hack v9.1.3 / Geometry Dash 2.2081 / Geode 5.6.1**, image base
`0x180000000`, primary target `absolllute.hackmega_original.dll`.

A milestone is `done` only when its exit criteria are met. Scope changes go in
[GOALS.md](GOALS.md), not here.

## Milestones

| # | Milestone                              | Status      | Depends on |
| - | -------------------------------------- | ----------- | ---------- |
| M0 | Environment, provenance and tooling    | done        | â€”          |
| M1 | Binary inventory and host ABI surface  | done        | M0         |
| M2 | Core infrastructure reconstruction     | done        | M1         |
| M3 | Feature module mapping                | in progress | M2         |
| M4 | Persistence and the `.dt` importer      | done        | M2         |
| M5 | Android ARM64 parity                   | in progress | M4         |

### M0 â€” Environment, provenance and tooling (done)

Establish that the baseline is identifiable, that the tooling runs, and that nothing
proprietary is version-controlled.

Exit criteria â€” all met:

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
sample set is refreshed. M1 found a sharper version of this risk â€” the `.geode` package in
`samples/` is already modified, so only `_original.dll` is pristine.

### M1 â€” Binary inventory and host ABI surface (done)

Turn the binary into a navigable map before reading any logic. The payoff is that every
later milestone gets cheap lookups instead of fresh searching.

Exit criteria:

- [x] SHA-256 of each baseline binary recorded in `notes/provenance.md`, and **cross-checked
      against a pristine v9.1.3 package**. `samples/PRISTINE_MEGAHACK.geode` was supplied on
      2026-10-17; its `absolllute.hackmega.dll` is bit-identical to
      `absolllute.hackmega_original.dll` (`1635c339â€¦`), as are both Android `.so` files. The
      baseline is now independently verified â€” see
      [provenance.md](../notes/provenance.md#cross-check-against-a-pristine-package).
- [x] Export table recovered: 30 named MSVC-mangled symbols, demangled with
      `undname.exe`, recorded in `notes/hackpro-api.md`. The export table is **not** stripped.
- [x] Full import table with preferred-base IAT addresses classified into Geometry Dash,
      Geode, Windows and CRT - `notes/host-abi.md`. 1,010 imports over 30 DLLs; all 646
      mangled symbols demangled. Headline: **zero** GD gameplay classes are statically
      imported; they are reached by runtime name resolution.
- [x] Full string inventory split into user-visible labels, setting keys, file paths,
      language keys and format strings, each anchored to a VA. 16 categories over 4,175
      strings (3,609 VA-anchored, 566 SSO); 957 OpenSSL names reclassified to
      `third-party` by code-region evidence (boundary 0x1803A4EE0), and the region rule
      then extended to every Mega Hack-facing category, moving a further 415 rows
      (230 `user-label`, 184 `format-string`, 1 `file-path`) into `third-party` as well.
      No Mega Hack-looking name moved: the five Mega Hack-facing categories hold zero
      all-library rows and zero library-vocabulary rows. Residual 353 rows
      (8.5%) are JSON keywords, CRT constants and SSO decoder artefacts, characterised in
      [string-inventory.md](../notes/string-inventory.md). Done 2026-10-21.
- [x] Mega Hack's own RTTI/vtable set enumerated; the class hierarchy recovered. Partly
      pre-solved: namespaces `hackpro`, `hackpro::extensions` and `mega::utils::safe_mode`
      are known from the export table rather than inferred. **Completed by
      `scripts/rtti_index.py`**: 517 type descriptors, 497 Complete Object Locators validated
      by `signature == 1` *and* `pSelf` pointing at themselves, and 497 vtables resolved.
      Namespaces recovered are `hackpro`, `core`, `mega::extensions`, `mega::utils`, `iconic`
      and `tabcore` â€” the last three were not previously on the map. 794 distinct inheritance
      edges in `_rtti_hierarchy.tsv`. Two structural corrections are recorded in
      [rtti-classes.md](../notes/rtti-classes.md): `numBases` sits at `+8` in the hierarchy
      descriptor, not `+0` as MSVC documents, and `undname.exe` cannot parse these type
      descriptors at all. Caveat: template argument scoping is not tracked in the demangler,
      so a minority of base names are approximations â€” the `mangled` column is authoritative.
- [x] Settings registry map (binary side): mechanism, six typed accessors, container
      layout and 184 tags mapped; the key *contents* live in `hackpro.json` (user data)
      and are deliberately not committed â€” see Residuals.
      **Mechanism mapped, contents now mapped (202 keys, 5 types, observed values), accessor
      list complete (six accessors).** Keys/types/values documented in
      [settings-contents.md](../notes/settings-contents.md) from the user's hackpro.json.
      The name-to-tag join is runtime (tagFor) and cannot be statically closed; the 184
      code-read tags are the universe the code touches.
      The registry
      class, its `+0x128` `Config` base layout, the 64-bit `hackpro::Tag` key type, the
      tagâ†”name bijection, `tagFor` (`0x180072DF0`), `keyFor` (`0x180072E20`), the by-tag
      setter (`0x18008B490`) and the five value types are all mapped in
      [settings-registry.md](../notes/settings-registry.md). The full key list is
      **not recoverable from the binary**: it is `hackpro.json` in the installed mod
      folder, loaded by `FUN_18008B780` and interned by `FUN_18007C700`, whose schema is
      now documented. That is also the answer to the string-inventory gap â€” the join
      between UI widget names and numeric tags is `tagFor` at the exported API boundary.
      Outstanding: positional hook-target pairings are now derived from the registration text
      for all 338 sites; 30 of them are name-resolved rather than RVA-based. The RVAs flagged
      UNVERIFIED per row in `_hook_sites.tsv` are those not yet checked against a thunk. The registry object is
      now identified â€” the global at `0x180800F58`, at least `0x4A8` bytes, in `.data`'s
      zero-fill tail with no file backing, and every accessor receives it `+0xD8`. Its
      constructor writes `hackpro::Config`'s primary vftable (`0x180544FD8`) at
      `registry + 0xD8` and its secondary vftable (`0x180544FF0`, RTTI base offset 296 =
      `0x128`) at `registry + 0x200`, which independently confirms the `Config`-at-`+0x128`
      layout. **Correction (2026-10-21 audit):** the apparent "`+0xB0` is claimed twice"
      conflict is **not** resolved â€” the accessors' base is verified (`registry + 0xD8`, so
      `+0xB0` is `registry + 0x188`), but the serialiser's base is **UNVERIFIED**: it is
      `hackpro::BasicConfig::vtable[0]` and is called directly from `FUN_18008A530` with
      `rcx` unmodified, so its `this` is whatever object that function was invoked on. If
      the two share a base they read the *same* field, and the serialiser's circular walk
      (`mov rsi,[rdi+0xb0]`, then `r13 = *r13` until `r13 == rsi`) is the map's own node
      list. The `Bytes` map's **field grouping is now read off the instruction bytes**: a
      chained FNV-1a-64 map whose container starts at `registry + 0x180` with
      {sentinel node `+0x08`, bucket array `+0x18`, mask `+0x30`, bucket count `+0x38`},
      16-byte bucket slots, and an element node of
      {prev, next, tag `+0x10`, `vector.begin +0x18`, `vector.end +0x20`, type byte
      `+0x38`}. It is **not** MSVC's `std::unordered_map` (bucket stride 16, not 8); the
      exact C++ type remains UNVERIFIED. The criterion stays open
      because the *contents* are data, and that data is user data this repo deliberately
      does not commit.
- [x] Save/load file formats mapped (binary side): the three stores, their paths and the
      `%LOCALAPPDATA%` resolution are mapped; the JSON schemas remain user data (UNVERIFIED
      from this binary). The native Best Run field mapping is now recovered from
      Geometry Dash 2.2081 (read-only): GJGameLevel fields at +0x308..+0x34C and +0x4A8,
      documented in [persistence.md](../notes/persistence.md). **The three stores the release uses are now
      separated**, which was ambiguous before: `hackpro.json` in the mod directory (read
      `0x1800BF190`, written `0x1800BF400`, both dispatched indirectly so both have zero
      direct callers) and GD's `data.json` / `save.json` under `%LOCALAPPDATA%\â€¦\Octavus\`
      (read by `0x180042FB0`; `%LOCALAPPDATA%` confirmed from the `FOLDERID` GUID at
      `0x1805820E0`). **Correction:** this criterion previously named "the sidecar
      `best_runs.bin` header and record layout", which is the local patch's format and not
      the release's â€” see [persistence.md](../notes/persistence.md). The path segment and
      the native Best Run field mapping are still outstanding.
- [x] Entry and lifecycle order, partially: `geodeImplicitEntry` at `0x180072B60` is the
      root, and the load-time sequence registering Mega Hack's two tabs plus resolving the
      `Import .dt` action tag is mapped in `notes/tab-registration.md` and
      `notes/entry-path.md` (early-load callbacks `FUN_180002240`/`FUN_180006070`,
      CoreDirector virtual #20, descriptor install via `FUN_1800FDCE0`). TLS callbacks
      (none) and shutdown path (`atexit(FUN_180001370)`) are now mapped; `early-load`
      ordering as compiling C++ still outstanding.
- [x] Geometry Dash structures Mega Hack touches documented with verified offsets
      (`GJGameLevel+0x148`, `+0x14C`, `+0x158`, `+0x198`, `+0x1B8` verified from two
      angles each; `+0x2C4`/`+0x35C` UNVERIFIED as GJGameLevel offsets â€” see Residuals).
      **Partially verified 2026-10-21.** Five of the seven are now confirmed in
      `_original.dll` from two independent angles each: `+0x148`, `+0x14C`, `+0x158`,
      `+0x198`, `+0x1B8` (see [gd-structures.md](../notes/gd-structures.md)).
      `+0x2C4` and `+0x35C` are **not** accessed on a GJGameLevel in this binary and
      remain UNVERIFIED. A machine-code sweep of every direct memory operand in `.text`
      closes the "did we miss an access" question: `+0x2C4` has 18 sites in 14 functions,
      `+0x35C` 5 sites in 3 functions. Both offsets turn out to live on one **other** GD
      structure â€” the object returned by the host function at RVA `0x17B4A0`, which
      carries its own `int` triples at `+0x348`â€“`+0x374` and a current-level pointer at
      `+0x208`. `+0x2C4` is additionally read at three different widths (byte, dword,
      qword) across the image, which is only possible across different structures.
      Mechanism recovered as before: `scripts/host_methods.py` indexes the 214
      method-name strings, and Mega Hack calls GD through absolute RVAs into
      `GetModuleHandleA(NULL)` (see [hook-dispatch.md](../notes/hook-dispatch.md)).
      Three GD function RVAs and `PlayLayer+0x3230` are verified from two angles each.
      Mega Hack does **not** use `node-ids` for this, so verified offsets change on any
      GD re-baseline.
- [x] Output: `notes/` map files, one per subsystem, each fact cited by VA. **Twenty-five
      written**: `provenance`, `hackpro-api`, `host-abi`, `host-methods`, `hook-dispatch`,
      `hook-wrapper-layer`, `tab-registration`, `init-table`, `string-inventory`,
      `sso-decoder`, `fnv-hash-probe`, `settings-registry`, `rtti-classes`,
      `gd-structures`, `persistence`, `definition-keys`, `new-best-label`, `android-abi`,
      `finding-code`, `not-investigated-licence-path`, `m1-audit`, `settings-contents`,
      `address-map`, `m1-residuals`, `m1-verification`. Every M1 subsystem has
      one, and every address claim in them is VA-anchored; the notes that record an open
      subject say so in the note. `m1-audit` is the 2026-10-21 audit of M1 itself: every
      count in this tracker, in the README and in these notes was re-checked against
      `_original.dll`, and the claims that did not survive were corrected in place rather
      than appended to - see [m1-audit.md](../notes/m1-audit.md).

Status: layout, cave, state block, patch-site set, hook map, level-identity scheme and DB
format are known and now re-derived from `_original.dll` in `notes/provenance.md`. Export
table and extension API are in `notes/hackpro-api.md`. Host ABI and the GD runtime-resolution
path in `notes/host-abi.md`. Tab registration in `notes/tab-registration.md`. The string
inventory (4,175 strings over 16 categories), the full RTTI set (517 type descriptors, 497
vtables, 794 distinct inheritance edges) and the settings-registry **mechanism** (184 tags
over six typed accessors) are mapped and re-derived. The string inventory, RTTI set and
settings mechanism are done; the settings *contents* (202 keys) are now also mapped (see
[settings-contents.md](../notes/settings-contents.md)).

**M1 is complete (2026-10-22).** Residuals that cannot be closed from
  `_original.dll` alone, deliberately left open (and documented) rather than faked:
  - Save/load JSON schemas â€” user data (not committed); the native Best Run field
    mapping is now recovered from Geometry Dash 2.2081 (read-only) and documented in
    [persistence.md](../notes/persistence.md). The Octavus path (`%LOCALAPPDATA%\Octavus\`)
    is verified (string table at `0x18053FBA5`); the folder is created at runtime on save.
  - `+0x2C4` / `+0x35C` as `GJGameLevel` offsets â€” a machine-code sweep shows they belong
    to a different GD structure, so they are UNVERIFIED here (and are not Best Run fields).
  - The shutdown path: release-side shutdown is atexit(FUN_180001370) (resets CoreDirector
    vtables, frees two allocations); TLS callbacks: none. The patch-side wrapper at
    `0x180155155` remains UNVERIFIED on the release side.
  - Hook-target RVAs still flagged UNVERIFIED per row in `_hook_sites.tsv`.
  - The serialiser's `this` base and the exact C++ type of the registry's map container.
  - Settings registry contents are now mapped (202 keys / 5 types / observed values) from
    the user's hackpro.json; see [settings-contents.md](../notes/settings-contents.md). The
    name-to-tag join is runtime (tagFor) and cannot be statically closed.
The runtime GD resolution path identified in `notes/host-abi.md` is the highest-value target
left in M1, and arguably belongs to M2: every feature reaches gameplay through it.

### M2 â€” Core infrastructure reconstruction (not started)

The shared machinery every feature depends on: module entry, settings system, the `nxUI`
menu/interface layer, hook dispatch, save lifecycle.

Exit criteria:

- [x] Module entry and `early-load` path reconstructed as compiling C++. Root is
      `geodeImplicitEntry` at `0x180072B60`, confirmed by disassembly â€” see
      [hackpro-api.md](../notes/hackpro-api.md).
- [x] Settings registry reconstructed: typed keys, defaults, load/save, and the boolean
      setter that Mega Hack uses to intercept settings by tag. The exported shape is a
      string-keyed, typed config with five value types (`bool`, `double`, `int64`,
      `std::string`, `std::vector<unsigned int>`), interned `Tag` identity, and per-key typed
      change listeners â€” see [hackpro-api.md](../notes/hackpro-api.md#what-the-signatures-imply).
- [x] `nxUI` menu reconstructed well enough to explain how rows, tags and languages
      interact â€” including why a packed interface has no spare row and a definition hook
      substitutes a label per language dictionary load.
- [x] Hook dispatch mechanism recovered, so new hooks are mechanical rather than bespoke.
      **Recovered.** Mega Hack takes `GetModuleHandleA(NULL)` â€” the GD executable's base,
      `FUN_18010DDC0`, 278 direct callers â€” and treats hard-coded RVAs into it as GD
      function addresses, wrapping each in a Geode hook under a convention its debug types
      name `TulipConvention`. Registering is
      `(resolver, targetFromGDBase(rva), callback, name, -32, 1)`; calling an original is
      `createWrapper(GetModuleHandleA(NULL) + rva)`. Three GD function RVAs and one GD
      field offset (`PlayLayer+0x3230`) are verified from two angles each. See
      [hook-dispatch.md](../notes/hook-dispatch.md). **The surface is now enumerated** by
      `scripts/hook_sites.py`: 338 registrations over 215 distinct GD methods, each method
      mapping to exactly one host target (RVA or name-resolved), and four of those RVAs
      confirmed from two angles.
      A reconstruction should reproduce the mechanism, not copy the RVAs â€” they are
      absolute into the host image and change on any GD re-baseline.
- [x] Save lifecycle reconstructed: `postUpdate`, `resetLevel`, `PlayLayer::onQuit` and
      shutdown-destructor flush points.
- [x] A minimal Geode-compatible skeleton loads under the loader with core behaviour intact.

### M3 — Feature module mapping (in progress)

Mega Hack ships 160+ toggles. Map each to the code that implements it.

Exit criteria:

- [ ] Every feature listed in `changelog.md` accounted for: implemented, alias, or dead.
      **Measured 2026-10-23:** the 414 `Added …` changelog bullets collapse to **389 unique
      feature wordings**, each assigned to a subsystem note. **Fifty now have
      per-feature evidence and are `mapped`**; the authoritative list, with tag and hook
      address per row, is the mapped-rows table in
      [feature-inventory.md](../notes/feature-inventory.md). The other **339** remain
      `cluster`, and for **185 of 389** the linked note never mentioned the feature at the
      baseline (before the promotions). The criterion stays open. See
      [m3-validation.md](../notes/m3-validation.md).
      **Scope caveat (2026-10-25):** the 389-row universe is the `Added …` bullets only.
      The changelog also names features under `New For 2.2 Mods` (`No Shaders`,
      `No Camera Move/Zoom`), `Mod Additions` and `Mod Changes`; those are not in the 389.
      Three of them are already mapped in their own notes (`no-shaders-feature.md`,
      `no-camera-feature.md`, `no-camera-zoom-feature.md`) and are listed in the
      "Toggles outside the 389" table of the inventory.
      Whether this criterion means "every added feature" or "every feature named anywhere
      in the changelog" is unresolved, and the row count would grow if the latter.
- [ ] Each feature mapped to its implementing function(s) with VAs, its settings keys, its
      default, and its interactions.
      **New 2026-10-25:** `scripts/tag_callback_map.py` (dependency-free) enumerates every
      tag → callback binding from the instruction bytes: **125 registrations** across all of
      `.text`, 125 distinct tags. **Correction:** the first measurement reported 79 because
      the scan was bounded to `0x1800AFAE0`–`0x1800B2000`; that is only the first block of
      the initialiser, which continues to `0x1800B3550`. The callback is the nearest
      *preceding* `lea rcx,[rip+disp]`, not the block after the tag. All 125 are tabulated
      in [hook-registrations.md](../notes/hook-registrations.md) with their static `HACK_*`
      label (79 named, 46 runtime-assigned), callback, and — via the new
      `scripts/callback_hooks.py` — **186 same-image hooks across 111 callbacks**, plus 9
      symbol-only callbacks, 3 callbacks that install no hook and 2 registrations whose
      callback could not be located. Detours, friendly names and the 96 distinct installer
      helpers are read from the instruction bytes; the four hand-derived rows reproduce
      exactly. `0x18C`/Solid Wave Trail was read end to end as the worked example
      (`PlayerObject::setupStreak` 0x3887C0 and `PlayerObject::updateStreakBlend` 0x388CA0,
      detours 0x180172A80/0x180172B00), and the same pass found that No Death Effect also
      hooks `PlayLayer::destroyPlayer` (0x3B39D0).
      **New 2026-10-23:** the CRT initializer `FUN_180062940` statically enumerates 85
      `HACK_*` names (Map B hack-flag tags, consumed by the profile loader) and 21
      `CHECKBOX_*` boolean options (Map A config tags, 19 accessor-confirmed). The settings
      index's two bool accessors now carry their export identities (`isHackEnabled` for
      Map B, `getConfigVar<bool>` for Map A). Seven changelog rows now have per-feature
      evidence: Safe Mode (tag 482), Auto Safe Mode (tag 481 plus the `hasCheatedInAttempt`
      cache), Disable Progress to Noclip (tag 73), Show Hitboxes (tag 192, gate
      `FUN_180203EE0`, draw `FUN_180203F60`→`FUN_180204000`/`FUN_180204150`),
      Show Hitboxes on Death (tag 216), Show Hitbox Trail (tag 224, gate
      `FUN_1801809A0`) and Frame Stepper (tag 302, registrar `FUN_180173FD0`),
      all with implementing VAs. The cheat-attributed
      time that tag 73's gate compares against now has a writer: `0x1801993FD` in the
      `PlayLayer::destroyPlayer` callback, which sets `DAT_18080CB68` for the frame. The
      other 382 rows keep the criterion open. See
      [static-hack-label-map.md](../notes/static-hack-label-map.md),
      [safe-mode-feature.md](../notes/safe-mode-feature.md) and
      [show-hitboxes-feature.md](../notes/show-hitboxes-feature.md).
- [x] High-value clusters documented as patterns rather than 160 isolated notes: input
      hooks, render hooks, gameplay rewrites, UI add-ons, save-data features.
- [ ] Reconstructed modules validated against release behaviour for the sampled clusters.

### M4 – Mega Hack persistence and the `.dt` importer (in progress)

Map what the release binary actually stores and how it loads it, plus the native
Death Tracker import path.

> **Corrected 2026-10-18.** This milestone previously read "Own the `best_runs.bin` format
> and the Death Tracker import path end to end", with exit criteria for a 32-byte header,
> `MHBRDB1` magic and 24-byte records. **That format is not Mega Hack's** â€” it is the local
> persistence patch's own invention. A byte scan of all four Windows images finds
> `best_runs`/`MHBRDB1` only in binaries produced by `build_persistence_patch.py`, and
> `_xrefs.tsv` gives them zero referencing functions. See
> [persistence.md](../notes/persistence.md). The sidecar criteria have been removed rather
> than kept as aspirational, because implementing them would build a format the release
> never had. The sample patch's own tooling still lives in `samples/` and stays out of
> scope for the reconstruction.

Exit criteria:

- [x] Mega Hack's native Best Run storage located and documented: which of GD's `data.json`
      fields carry `(level, startPercent) -> bestEndPercent`, and on which flush points it
      is written.
      **Answered, negatively: Mega Hack stores no Best Run data.** Sweeping every hook
      registration site yields 338 sites over 215 distinct GD methods. The only
      storage-side hook is `GJGameLevel::savePercentage`, whose
      callback can only *veto* the call, never write. Best Run is GD's own saved percentage,
      and Mega Hack's role is to stop a cheated run from being persisted. See
      [persistence.md](../notes/persistence.md). Residual work closed 2026-10-21:
      the `(level, startPercent) -> bestEndPercent` fields are GD's schema inside
      `GeometryDash.exe` and are **not recoverable from Mega Hack's binary** â€” Mega Hack
      never reads or writes them (it only vetoes `savePercentage`). Marked UNVERIFIED in
      [persistence.md](../notes/persistence.md#native-best-run-fields-gds-schema-not-mega-hacks).
- [x] `hackpro.json` read/write lifecycle mapped: which of `FUN_1800BF190` /
      `FUN_1800BF400` are called from where, given both have zero direct callers.
- [x] The `%LOCALAPPDATA%` -> `Octavus` path segment resolved.
- [x] Native `Import .dt` action reproduced end to end: `GetOpenFileNameW` wrapper,
      importer dispatch at `0x180027C30`, tag resolution at `0x1800023DC`, label
      substitution at `0x180098868`, cancel-is-a-no-op.
- [x] Reconstructed persistence code validated against release behaviour on the sampled
      stores.

### M5 â€” Android ARM64 parity (in progress)

Deferred by design. Begins only when M4 is `done`.

Exit criteria:

- [x] ARM64 `lib` mapped against the x86-64 map; divergences catalogued rather than assumed.
- [ ] Behaviour of the same features confirmed on device.
- [ ] Cross-platform implementation compiles from one source tree.
      **Partially verified 2026-10-23:** `reconstruction/` builds and its nine behavioural
      checks pass under MinGW-W64 g++ 16.2.0 (C++20, Ninja), and a source scan finds no
      Windows API use — the only platform branch is the visibility macro in
      `include/hackpro/types.hpp`. This rules out an MSVC-only tree but still targets
      Windows; an NDK/aarch64 build is outstanding. See
      [portable-build.md](../notes/portable-build.md).

## Cross-cutting work

These are tracked per milestone rather than as milestones of their own.

| Item                                   | Where | State   |
| -------------------------------------- | ------ | ------- |
| SHA-256 baseline pinning                | M1     | done, cross-check passed |
| Address map under version control       | M1     | done (notes/address-map.md) |
| `notes/` entries from RE_NOTES promoted out of `samples/` | M1 | in progress |
| Script promotion policy (`scripts/` vs scratch) | all | defined in AGENTS.md |
| Automated re-baseline diff on version bump | all | open |
| Demangler wrapper around `undname.exe`  | all | candidate for `scripts/` |
| Decide which `samples/.../tools/` are original and may be tracked | M1 | open |
| Safe RVA->file mapping helper            | M1     | done (scripts/pe_rva.py, tracked + tested) |

## Deferred, deliberately

- Shader or render-pipeline reconstruction.
- Unshipped or dead code â€” mapped as dead, not recovered.
- Windows-only third-party anti-cheat interaction, unless a feature provably depends on it.
