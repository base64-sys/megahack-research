# M1 verification — independent re-derivation

- **Date** — 2026-10-22.
- **Scope** — M1 only. No M2 work started. Nothing committed or pushed.
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`
  (SHA-256 `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`),
  base `0x180000000`. RVA → file offset only via `scripts/pe_rva.py`.
- **Method** — every count and address below was re-derived from the binary this session,
  not read from notes. Two independent angles were used where the task required them.

## Phase 1 — counts re-derived from `_original.dll`

| # | Check | Expected | Re-derived | Result |
|---|-------|----------|------------|--------|
| 1 | SHA-256 `_original.dll` vs DLL inside `PRISTINE_MEGAHACK.geode` | match | `1635c339…` both | PASS |
| 1 | Folder copy `absolllute.hackmega.dll` | must NOT match | `24cebcac…` | PASS |
| 1 | `samples/` tracked | none | `git ls-files samples recovered` = 0 | PASS |
| 2 | `classify_strings.py` rows | 4,175 | 4,175 | PASS |
| 2 | categories | 16 | 16 | PASS |
| 2 | VA / SSO | 3,609 / 566 | 3,609 / 566 | PASS |
| 2 | `other` | 8.455% (353) | 353 (8.455%) | PASS |
| 2 | `third-party` | 2,253 | 2,253 | PASS |
| 2 | 5 MH-facing categories: all-library rows | 0 | 0 | PASS |
| 2 | 5 MH-facing categories: library-vocabulary rows | 0 | 0 | PASS |
| 3 | `rtti_index.py` type descriptors | 517 | 517 | PASS |
| 3 | COLs | 497 | 497 | PASS |
| 3 | vtables | 497 | 497 | PASS |
| 3 | raw edges | 3,088 | 3,088 | PASS |
| 3 | distinct edges | 794 | 794 | PASS |
| 3 | derived classes | 297 | 297 | PASS |
| 3 | `numBases` at `+8` | yes | read at a known COL: `+0=0, +4=0, +8=2, +12=array RVA` | PASS |
| 3 | `undname.exe` failure | yes | echoes `.?AVCheckbox@ui@hackpro@@` unchanged | PASS |
| 4 | `settings_index.py` distinct tags | 184 | 184 | PASS |
| 4 | total reads | 233 | 233 | PASS |
| 4 | range | 1..883 | 1..883 | PASS |
| 4 | keyed lookups | 4 | 4 | PASS |
| 4 | accessors | exactly 6 | 6 (`ACCESSORS` dict) | PASS |
| 5 | `init_table.py` rows | 283 | 283 (header is `#`-comment) | PASS |
| 6 | imports | 1,010 | 1,010 | PASS |
| 6 | distinct DLLs | 30 | 30 | PASS |
| 6 | mangled | 646 | 646 | PASS |
| 6 | exports | 30 named (0 ordinal-only) | 30 named, 0 ordinal-only | PASS |
| 7 | every address in `notes/address-map.md` | file-backed (except documented zero-fill/gap) | 27 backed, 3 zero-fill, 1 gap; only offsets/bases unmapped | PASS |

## Phase 2 — contradictions found and resolved

| # | Claim | Where | Finding | Resolution |
|---|-------|-------|---------|------------|
| 1 | "20 notes" | `notes/m1-audit.md` (two sites) | `notes/` holds 24 `.md` files; `docs/ROADMAP.md` lists 24 by name | Corrected `m1-audit.md` → 24 notes |
| 2 | "0 ordinal-only imports" | `notes/m1-audit.md` | Raw PE import table has **27** ordinal imports, all `WS2_32.dll`. `pefile` reports 0 because its `ordlookup` resolves ordinals to names. Both readings are defensible; the raw-bytes reading is 27 | Recorded both readings; `m1-audit.md` now states 27 by ordinal and 0 unnamed |
| 3 | M1 heading "(in progress)" | `docs/ROADMAP.md` line 44 | Table row 14 marks M1 `done` and the section ends "M1 is complete (2026-10-22)" | Corrected heading to "(done)" |
| 4 | Mangled-name count vs exports | `notes/hackpro-api.md` | 30 named exports confirmed; 26 in the API block + 3 `safe_mode` + 1 `define` | Consistent, no change |
| 5 | `+0xB0` "claimed twice" | `settings-registry.md`, `hook-dispatch.md` | Accessors' base verified (`registry + 0xD8`); serialiser's base UNVERIFIED | Already documented as UNVERIFIED; left as-is |
| 6 | Bytes map at `+0x170`/`std::unordered_map` | `settings-registry.md`, `hook-dispatch.md` | Custom chained FNV-1a-64 map at `registry + 0x180` | Already corrected in notes |

No other numeric mismatches were found across the 14 notes, `README.md` and `docs/ROADMAP.md`
for the checked quantities (DLL counts, category counts, accessor counts, tag/read counts,
edge counts, note counts, offset claims).

## Phase 3 — GD-side verification (read-only)

- `GeometryDash.exe` at `C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\GeometryDash.exe`:
  image base `0x140000000` confirmed; PE timestamp `2026-01-21 01:36:32`. No `2.2081`
  string is present in the image (version not string-verifiable from this binary; the user's
  recorded version is relied upon).
- `GJGameLevel::savePercentage` at RVA `0x16C8B0` resolves to a function prologue
  (`mov rax, rsp` …). Field accesses present: `+0x308`, `+0x30C`, `+0x320`, `+0x324`,
  `+0x328`, `+0x32C`, `+0x330`, `+0x340`, `+0x344`, `+0x348`, `+0x34C` and `+0x4A8`.
  Practice flag: `test r8b, r8b` at `0x14016C8FF` with the `je`/`jne` split documented.
  `ebp < edi` branch: `cmp ebp, edi` at `0x14016C997` with `jge` to the skip path.
- `PlayLayer::resetLevel` (`0x3B8EB0`) and `PlayLayer::levelComplete` (`0x3A7A80`) both
  resolve to function prologues (`mov rax, rsp`). Neither references the GJGameLevel Best Run
  fields `+0x308..+0x34C`: the single hit in `resetLevel` is `lea rcx, [rdi + 0x328]` on the
  `PlayLayer` object (`rdi`), not on a `GJGameLevel`; the hit in `levelComplete` is a stack
  slot `[rsp + 0x320]`.
- `+0x2C4` and `+0x35C` are not referenced anywhere in `savePercentage` (grep of the
  disassembly, 0 hits).

## Phase 4 — scripts and validators

- `test_pe_rva.py`, `test_settings_index.py`, `test_init_table.py`, `test_progress_svg.py`,
  `make_progress_svg.py --check` all pass.
- `scripts/` contains no one-off audit scanners (they live in `.oc/audit/`).
- `pefile.get_offset_from_rva` / linear RVA subtraction appear in tracked files only in prose
  explaining why they are wrong (`AGENTS.md`, `notes/tab-registration.md`,
  `scripts/pe_rva.py`, `scripts/resolve_wrappers.py`'s VA→RVA conversion of recovered
  filenames, which is not a file-offset conversion).
- `.gitignore` blocks `*.dll`, `*.exe`, `samples/`, `recovered/`, `.oc/`.

## Phase 5 — samples hygiene

- `git status --porcelain`: no `samples/` or `recovered/` entries.
- `git ls-files samples recovered` = 0.
- No binary tracked (`git ls-files | grep -E '\.(dll|so|exe|geode)$'` empty).
- `_original.dll` SHA-256 still matches the pristine package.

## Phase 6 — M1 exit criteria vs. evidence

| Criterion | Evidence location | State |
|-----------|-------------------|-------|
| Provenance (SHA cross-check) | `notes/provenance.md` | VERIFIED |
| Export table (30 named) | `notes/hackpro-api.md` | VERIFIED |
| Import table (1,010 / 30 / 646) | `notes/host-abi.md` | VERIFIED (caveat: 27 by ordinal, see above) |
| String inventory (16 categories) | `notes/string-inventory.md` | VERIFIED |
| RTTI / vtables (517 / 497 / 497) | `notes/rtti-classes.md` | VERIFIED |
| Settings registry (binary side) | `notes/settings-registry.md`, `settings-contents.md` | VERIFIED on binary side; contents are user data |
| Save/load (binary side) | `notes/persistence.md` | VERIFIED on binary side; JSON schemas are user data |
| Entry / lifecycle (partial) | `notes/tab-registration.md` | VERIFIED (partial, as documented) |
| GD structures | `notes/gd-structures.md` | VERIFIED for 5 of 7; `+0x2C4`/`+0x35C` UNVERIFIED |
| Output (notes) | `notes/` (24 files) | VERIFIED |

The four criteria the task singled out (settings registry, save/load, GD structures, output)
are ticked in `docs/ROADMAP.md` and the evidence supports keeping them ticked: the residuals
they name (user data, UNVERIFIED offsets) are documented, not hidden.

## Claims that could NOT be re-verified

- **GD version 2.2081** — no version string exists in the supplied `GeometryDash.exe`; only
  the PE timestamp (`2026-01-21`) is available. Confirm via the user's Steam install
  metadata or the game's in-app version display.
- **Serialiser's `this` base** in `settings-registry.md` — disassembly admits two readings;
  needs a dynamic/runtime observation to settle. Remains UNVERIFIED.
- **Exact C++ type of the registry's map container** — not `std::unordered_map`; the precise
  type is not recoverable from the binary. Remains UNVERIFIED.
- **The 27 `WS2_32` ordinal imports' names** — recoverable via the standard ordinal table
  (pefile does this); not needed for M1.
- **Hook-target RVAs** flagged UNVERIFIED per row in `_hook_sites.tsv` — not re-checked here.

## Final verdict

**Yes, with caveats.** M1's binary-side inventory is internally consistent and re-derives
cleanly from `_original.dll`: every count matches, the address map resolves, the validators
pass, and the GD-side RVAs resolve to prologues with the documented field accesses. The
caveats are the already-documented residuals (user data, UNVERIFIED offsets, serialiser base,
map container type) plus two corrections made during this pass (note count 20 → 24; the
ordinal-import reading). None of them invalidates a fact M2 will depend on, but M2 should
treat the UNVERIFIED items as UNVERIFIED rather than as settled inputs.