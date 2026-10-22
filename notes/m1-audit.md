# M1 audit — 2026-10-21

## Question

Are the M1 exit criteria in [docs/ROADMAP.md](../docs/ROADMAP.md) honest?

Every count, every address and every tick in the tracker, in the README and in the notes was
re-checked against `_original.dll` — the one file in the sample set that is bit-identical to
the shipped v9.1.3 release. Claims that did not survive were **corrected in place**, not
appended to, so there is exactly one version of each truth.

Scope: **M1 only**. No M2 work was started. Nothing was committed or pushed.

## Ground truth and toolchain

| | |
| --- | --- |
| Ground truth | `samples/absolllute.hackmega/absolllute.hackmega_original.dll` |
| SHA-256 | `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92` |
| Size | 8,697,056 bytes |
| Cross-check | `samples/PRISTINE_MEGAHACK.geode` → `absolllute.hackmega.dll`, **same digest** |
| Not ground truth | `absolllute.hackmega.dll` in the same folder = `24cebcac…` — the *generated* persistence-patch build |
| Python | 3.14.6 (`C:\Users\Robert_\AppData\Local\Python\bin\python3.exe`) |
| RVA → file offset | `scripts/pe_rva.py`, and nothing else |

Two rules held for the whole audit:

* **No linear subtraction, no `pefile.get_offset_from_rva`.** `git grep` over the tracked tree
  finds both names only in prose that explains why they are wrong
  (`scripts/pe_rva.py:11`, `notes/tab-registration.md:267`, `AGENTS.md:130`). Every address in
  this audit was read through `Image.from_path(...).read(va, n)`.
* **Audit tooling stayed out of `scripts/`.** The disassembly, RIP-reference and
  displacement scanners used here live in `.oc/audit/` (gitignored) and one-off scripts in
  the temp directory. `scripts/` only gained changes to files that already had a second,
  independent use — see Part 3.

## Part 1 — the five criteria M1 already claims as met

| # | Criterion | Tracker claim | Re-verified | Verdict |
| - | --------- | ------------- | ----------- | ------- |
| 1 | Provenance | SHA-256 recorded, cross-checked against a pristine package | digest reproduced from the archive; byte-identical | **VERIFIED** |
| 2 | Exports | 30 named MSVC-mangled symbols, demangled with `undname.exe` | 30 export rows re-read from the image | **VERIFIED** |
| 3 | Imports | 1,010 imports over 30 DLLs, 646 mangled | re-counted from the image: 1,010 / 30 / 646 / 27 imported by ordinal (WS2_32) | **VERIFIED** |
| 4 | Strings | 16 categories over 4,175 strings (3,609 VA, 566 SSO) | re-classified; category totals re-summed | **VERIFIED**, one claim corrected |
| 5 | RTTI | 517 TDs, 497 COLs, 497 vtables, 794 edges, 2 structural corrections | re-walked; counts unchanged | **VERIFIED** |

### 1. Provenance

`Get-FileHash -Algorithm SHA256` over the sample set and `Expand-Archive` over
`samples/PRISTINE_MEGAHACK.geode` (re-run this session):

```
1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92  absolllute.hackmega_original.dll
1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92  absolllute.hackmega.dll  (inside the pristine package)
24cebcac315c0d3e7ff26a0515970b2fcade98c8f665ebe825d7da0b8aa47996  absolllute.hackmega.dll  (in samples/absolllute.hackmega/)
```

The package's DLL and `_original.dll` are bit-identical; the folder's copy is the *generated*
persistence-patch build, which is why `AGENTS.md` names `_original.dll` as ground truth. Both
Android `.so` files in the package also match the extracted copies. The provenance note
already said all of this; the audit confirms it rather than changing it.

### 2. Exports

`pe_exports.py` (in `samples/absolllute.hackmega/tools/`, run with
`PYTHONPATH=samples/absolllute.hackmega/.analysis_tools`) lists **30** rows, no header, no
ordinals, mangling intact — e.g. `??$getConfigVar@N@extensions@hackpro@@…` and
`geodeImplicitEntry`. Demangling with `undname.exe`
(`C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\undname.exe`)
gives 26 symbols in the API block `0x180072B60`–`0x180075530`, 3 `mega::utils::safe_mode`
functions, and `hackpro::define` at `0x1800A3C40`. Second angle on the addresses: the export
VAs fall inside the module's `.text` range and were sampled with `pe_disasm.py`.

### 3. Imports

`pe_imports.py` re-run this session over `_original.dll`:

| | |
| --- | ---: |
| import rows | **1,010** |
| distinct DLLs | **30** |
| MSVC-mangled symbols | **646** |
| imports by ordinal (WS2_32, resolved to names by the standard ordinal table) | **27** |
| unnamed imports (no name in the PE import table) | **0** |
| delay-import directory | absent |

The headline claim — **zero GD gameplay classes are statically imported** — survives: every
GD entry point is reached by runtime name resolution, not by the import table.

One trap is recorded in `notes/host-abi.md` and was confirmed while re-reading: `imp.address`
is an **absolute VA** (e.g. `SHGetKnownFolderPath` at `0x180644580`), not an RVA. Reading it
as an RVA silently points into the wrong section.

### 4. Strings

`scripts/classify_strings.py` re-run over `_strings.tsv` (4,175 rows; three whitespace-only
rows are dropped by the classifier, leaving 3,609 VA-anchored and 566 SSO-only).

**Correction applied during this audit.** The region rule that separates Mega Hack's own
strings from the statically linked library (boundary `0x1803A4EE0` — a string is library-owned
when every function referencing it lives at or beyond that address) was originally applied only
to `mod-identifier` and `other`. Extending it to every Mega Hack-facing category moved a
further **415 rows** into `third-party`:

| Moved | Rows |
| ----- | ---: |
| `user-label` | 230 |
| `format-string` | 184 |
| `file-path` | 1 |

Resulting totals: **16 categories**, `third-party` **2,253**, `other` **353** (8.455%). The
five Mega Hack-facing categories hold **zero** all-library rows and **zero**
library-vocabulary rows — the check that says the rule did not eat real Mega Hack content.
`_xrefs.tsv` holds 4,178 rows.

### 5. RTTI

`scripts/rtti_index.py` output unchanged: **517** type descriptors, **497** Complete Object
Locators, **497** vtables resolved, **3,088** raw inheritance edges, **794** distinct
(derived, base) pairs, **297** derived classes with at least one base. The two structural
corrections recorded in `notes/rtti-classes.md` still hold:

1. `numBases` sits at **`+8`** in the BaseClassDescriptor, not `+0` as MSVC's documentation
   says.
2. `undname.exe` cannot parse these type descriptors at all, so the demangler's output for
   template arguments is approximate — the `mangled` column is authoritative.

The correctness gate is unchanged and is the reason the numbers are trustworthy: a COL
candidate must have `signature == 1` **and** `pSelf` pointing at itself. A 24-byte window
with the right signature occurs by chance in 8.7 MB of `.rdata`; one that also points at
itself does not. All 497 pass both.

## Part 2 — the four criteria M1 still lists as open

| # | Criterion | Tracker state | Audit verdict |
| - | --------- | -------------- | ------------- |
| A | Settings registry map | open | **OPEN for the right reason** — the *contents* are data, not code. Mechanism re-verified; two claims corrected. |
| B | Save/load file formats | open | **OPEN** — mechanism and file identities mapped; schema still unknown. |
| C | GD structures | open | **OPEN** — 5 of 7 offsets verified from two angles each; `+0x2C4`/`+0x35C` UNVERIFIED as GJGameLevel offsets. |
| D | Output (`notes/`) | ticked | **VERIFIED** – 24 notes at audit time, 25 in total (`m1-verification.md` was added in the same commit; `docs/ROADMAP.md` now lists twenty-five), every address claim VA-anchored; corrections applied. |

### A. Settings registry — mechanism verified, two claims corrected

The registry is the global object at `0x180800F58` (at least `0x4A8` bytes, in `.data`'s
zero-fill tail with no file backing), built by the static constructor `FUN_1800A27E0`
(`0x1800A27E0`), called as `FUN_1800A27E0(&DAT_180800F58)` from `FUN_1800A2720` and from
`hackpro::define` (`0x1800A3C40`). Every accessor receives that global `+0xD8`
(`FUN_18008A510` → `FUN_1800A2F90`, which is `lea rax, [rcx + 0xd8]; ret`).

Counts re-derived from `scripts/settings_index.py`:

| | |
| --- | ---: |
| distinct numeric tags | **184** |
| total tag reads | **233** |
| tag id range | **1 … 883** |
| keyed lookups (`FUN_18007CB60`) | **4** |
| `_settings.tsv` data rows | **188** |
| typed accessors | **6** |

The six accessors: `FUN_18008F420` (`isHackEnabled`, Map B), `FUN_18002DE50`
(`getConfigVar<bool>`, Map A), `FUN_180003D80` (`getConfigVar<scalar>`),
`FUN_18002E4C0` (`getConfigVar<int>`), `FUN_18002D710` (`getConfigVar<std::string>`),
`FUN_18002EE40` (`getConfigVar<hackpro::Bytes>`). `FUN_18002EE40` is identified by the literal it emits on a
miss — the requested type's own name, `"hackpro::Bytes"` — and by the type byte `4`
(`V_BYTES`) it requires at `node + 0x38`.

**Correction 1 — the `Bytes` map is not `std::unordered_map`, and not at `+0x170`.**
`notes/settings-registry.md` and `notes/hook-dispatch.md` both carried the claim that the
container is "the MSVC `std::unordered_map<uint64_t, std::vector<unsigned int>>` that
`FUN_1800A28C0` constructs at `object + 0x170`". The instruction bytes refute it:

| VA | Instruction | Meaning |
| -- | ----------- | ------- |
| `0x18002EEEE` | `and r8, qword ptr [rcx + 0xd8]` | bucket mask → `registry + 0x1B0` |
| `0x18002EEF5` | `mov rdx, qword ptr [rcx + 0xb0]` | chain sentinel node → `registry + 0x188` |
| `0x18002EEFC` | `mov rcx, qword ptr [rcx + 0xc0]` | bucket array base → `registry + 0x198` |
| `0x18002EF03` | `shl r8, 4` | **bucket stride 16 bytes** |
| `0x18002EF07` | `mov r14, qword ptr [rcx + r8 + 8]` | first node of the chain, `slot + 8` |
| `0x18002EF15` | `cmp rax, qword ptr [r14 + 0x10]` | the tag, `node + 0x10` |

MSVC's bucket array holds **8**-byte node pointers, its `_Hash_table` keeps the list head
inline rather than behind a pointer, and it has no mask member. Here the stride is 16, the
head is a pointer at container `+0x08`, and the mask is at `+0x30`. The container the
accessor reads starts at **`registry + 0x180`**, not `+0x170`.

Two independent confirmations of that shape: the constructor `FUN_1800A42A0`
(`0x1800A42A0`) writes the same layout for another instance of the type (`1.0f` at `+0x00`,
a 0x30-byte self-linked node at `+0x08`, a 0x80-byte bucket buffer at `+0x18` = 8 slots ×
16 bytes, `7` at `+0x30`, `8` at `+0x38`); and the string-keyed lookup `FUN_18007CB60`
(`0x18007CB60`) has the identical relative layout (mask at `+0x88`, sentinel at `+0x60`,
buckets at `+0x70`, same `shl r10, 4` stride), so its container starts at
`registry + 0x130`. It is Mega Hack's own chained FNV-1a-64 map.

**Correction 2 — the "`+0xB0` is claimed twice" conflict is not resolved.**
The accessors' base is verified (`registry + 0xD8`, so their `+0xB0` is `registry + 0x188`).
The serialiser's base is **not**: `FUN_180076EB0` is `hackpro::BasicConfig::vtable[0]`
(the image holds `0x18053E8D8 → 0x180076EB0`) and is called directly from `FUN_18008A530`
with `rcx` unmodified (`0x18008A55D`), so its `this` is whatever object that function was
invoked on. Under the old reading (`this = registry + 0x128`) the two readers hit different
fields; if they share a base they hit the **same** field, and the serialiser's circular walk
(`mov rsi,[rdi+0xb0]`, then `r13 = *r13` until `r13 == rsi`) is the map's own node list.
Both readings fit the disassembly; the notes now say so instead of asserting one.

The criterion stays **open** because the *contents* of the registry are `hackpro.json` in the
installed mod folder — user data this repo deliberately does not commit.

### B. Save/load — three stores, identities mapped, schema unknown

| Store | Path | Reader / writer |
| ----- | ---- | --------------- |
| `hackpro.json` | the mod folder | read `0x1800BF190`, written `0x1800BF400` (both dispatched indirectly — zero direct callers) |
| `data.json` / `save.json` | `%LOCALAPPDATA%\Octavus\` | read by `0x180042FB0`; the path is built by `0x180042FB0` from the folder returned by `0x1800C5210` → thunk `0x1805324F0` → `SHGetKnownFolderPath` |
| `best_runs.bin` | — | **not** the release's format; see the correction below |

The `FOLDERID` GUID `f1b32785-6fba-4fcf-9d55-7b8e7f157091` sits at `0x1805820E0`, and
`FUN_1800C5210` passes `FOLDERID_LocalAppData` to `SHGetKnownFolderPath`, so the path segment
is `Octavus` directly, with no `<sub>` directory in between. The `best_runs.bin` sidecar
documented in the sample tree's `RE_NOTES_PERSISTENT_BEST_RUN.md` is the **local patch's**
format, not the release's — a byte scan of all four Windows images finds `best_runs` and
`MHBRDB1` only in binaries this repo generates; `_original.dll` contains neither and
`_xrefs.tsv` gives them zero references.

Still open: the JSON schemas themselves, and the native Best Run field mapping.

### C. GD structures — five of seven verified, two UNVERIFIED

Verified from two independent angles each, in `_original.dll`:

| Offset | First angle | Second angle |
| ------ | ----------- | ------------ |
| `+0x148` | `FUN_18020BD00` `0x18020BDD3` `mov ecx, dword ptr [rax + 0x148]` | `FUN_180153CD0` `0x180153D73` on the object at `LevelPage + 0x1A8` |
| `+0x14C` | `FUN_18020BD00` `0x18020BDD9` `sub ecx, dword ptr [rax + 0x14c]` | `FUN_180153CD0` `0x180153D79` |
| `+0x150` | `FUN_18020BD00` `0x18020BDDF` (derived: `+0x148 - +0x14C`) | same object, same function |
| `+0x158` | `FUN_18020BD00` `0x18020BDF2` `lea r9, [rax + 0x158]` | `FUN_180145430` `0x180145464` `movups xmmword ptr [rsi + 0x158], xmm0` |
| `+0x198` | `FUN_180145430` `0x18014549E` | same function sets `GJGameLevel::vftable` (`0x180572C48`) at `[rsi]` |
| `+0x1B8` | `FUN_180145430` `0x1801454BB` | same as `+0x198` |

Plus `PlayLayer + 0x3230`, and three GD function RVAs confirmed twice (registration text and
a lazy thunk that re-reads `GetModuleHandleA(NULL) + rva`): `PlayLayer::resetLevel`
`0x3B8EB0`, `PlayLayer::levelComplete` `0x3A7A80`, `GJGameLevel::savePercentage` `0x16C8B0`.

**`+0x2C4` and `+0x35C` stay UNVERIFIED**, and the sweep is what makes that honest rather than
merely unfinished. A machine-code scan of every direct memory operand in `.text` finds
`+0x2C4` at **18 sites in 14 functions** and `+0x35C` at **5 sites in 3 functions**. Both
offsets live on one **other** GD structure — the object returned by the host function at RVA
`0x17B4A0`, which carries its own `int` triples at `+0x348`–`+0x374` and a current-level
pointer at `+0x208`. `+0x2C4` is additionally read at three different widths (byte, dword,
qword) across the image, which is only possible across different structures, and the only
comparison against it anywhere is `cmp edx, dword ptr [rcx + 0x2c4]` at `0x1800F1B55`.

### D. Output – the notes count, corrected

`notes/` holds 25 files, one per subsystem, every M1 subsystem covered (24 when this
section was corrected from 20; `m1-verification.md` was added in the same commit, and
`docs/ROADMAP.md` now lists twenty-five).
The audit changed nine of them and two documents; the list is in "Contradictions found and
fixed" below.

## Part 3 — cross-cutting

### One RVA → file-offset path

`scripts/pe_rva.py` is the only converter in the repo, and it is dependency-free and
tracked. `scripts/test_pe_rva.py` pins the behaviour that matters: a zero-fill runtime
pointer slot has no file bytes, the patch state block at `0x18081BFC0` is past
`VirtualSize` (a gap, not file-backed), backed RVAs agree with linear arithmetic, file
offset → VA round-trips at both section ends, `read()` refuses unbacked addresses, and
malformed images are rejected.

### Script promotion and de-promotion

Promoted (already had a second, independent use):

| Script | Change |
| ------ | ------ |
| `scripts/classify_strings.py` | region rule extended to every Mega Hack-facing category |
| `scripts/settings_index.py` | `FUN_18002EE40` relabelled `getConfigVar<hackpro::Bytes>`; the accessor table keeps an explicit argument index per accessor |
| `scripts/test_settings_index.py` | pins the `Bytes` row (tag 285) and the argument splitter |
| `scripts/init_table.py` | header changed to a `#`-prefixed comment line so all indexes count rows the same way (283) |

Re-run to refresh the indexes (the outputs live in the untracked `recovered/` tree):
`scripts/hook_sites.py` — 338 sites over 215 methods, 30 name-resolved; the on-disk
`_hook_sites.tsv` had been stale at 43 rows.

Not promoted: the audit's own scanners (`.oc/audit/disasm.py`, `rip_refs.py`,
`scan_disps.py`, `func_disps.py`, `at.py`) stayed in the gitignored `.oc/audit/` tree, and
one-off scripts went to the temp directory. A script that is used once does not go in
`scripts/`.

### `samples/` hygiene

`git status --porcelain` lists only tracked text files (9 notes, `README.md`,
`docs/ROADMAP.md`, 4 scripts) — no binaries, no extracted assets, no `samples/` entries.
`git ls-files samples recovered` returns **0** files: the sample set and the whole
`recovered/` tree are untracked by design. Hashes are recorded; bytes are not.

## Part 4 — documents corrected

| Document | Correction |
| -------- | ---------- |
| `docs/ROADMAP.md` | imports 32 → **30** DLLs; string criterion 17 → **16** categories and the 415 moved rows; settings criterion: vtable addresses, the `+0xB0` conflict downgraded to UNVERIFIED, the `Bytes` map's verified shape; GD criterion cites the machine-code sweep and stays unticked |
| `README.md` | 17 → **16** categories; third-party 2,253 and the 415 moved rows; `+0x170` vtable claim replaced with the two real vtable addresses; the "four accessors / four value types" → **six accessors / five value types**; the `+0xB0` trap paragraph corrected; `_settings.tsv` 184 → **188** rows |
| `notes/*.md` | nine notes, see below |

`docs/GOALS.md` was not touched.

## Part 5 — verification run

```
scripts/test_pe_rva.py          all 8 address cases plus 3 invariants passed
scripts/test_settings_index.py  all checks passed
scripts/test_init_table.py      all checks passed
scripts/test_progress_svg.py    all checks passed
scripts/make_progress_svg.py --check   no stale graphic
```

`git status` afterwards: only the intended tracked files.

## Contradictions found and fixed

| # | Claim | Where | Outcome |
| - | ----- | ----- | ------- |
| 1 | 32 import DLLs | ROADMAP | **30** (re-counted from the image) |
| 2 | 17 string categories | ROADMAP, README | **16** |
| 3 | `Bytes` map is MSVC `std::unordered_map` at `+0x170` | settings-registry, hook-dispatch | **Wrong** — a custom chained FNV-1a-64 map at `registry + 0x180`; corrected in both notes |
| 4 | "`+0xB0` is claimed twice" is **resolved** | settings-registry, hook-dispatch, README, ROADMAP | **Not resolved** — the accessors' base is verified, the serialiser's is UNVERIFIED |
| 5 | Constructor writes `hackpro::Config::vftable` at `+0x00` and `+0x128` | settings-registry, README, ROADMAP | **Bases were mixed** — rewritten with the registry global as the single base: primary vftable `0x180544FD8` at `registry + 0xD8`, secondary `0x180544FF0` at `registry + 0x200` (RTTI base offset 296) |
| 6 | 134 / 149 settings tags | settings-registry | **184** tags, 233 reads, range 1…883 |
| 7 | 138 settings tags | init-table | **184** |
| 8 | 29 exports in the API block | hackpro-api | **26** |
| 9 | 3,088 inheritance edges / 794 "edges" conflated | rtti-classes | **3,088 raw, 794 distinct**, 297 derived classes |
| 10 | 214 host methods / 32 DLLs | host-methods, host-abi | 214 methods ✓; DLLs **30** |
| 11 | `_settings.tsv` = 184 rows | README | **188** rows (184 tags + 4 keyed lookups) |

Two more notes changed without a numeric claim changing: `notes/gd-structures.md` (the
`+0x2C4`/`+0x35C` sweep tables, added) and `notes/string-inventory.md` (16-category table,
region-rule section).

## Still open after this audit

* The settings registry **contents** — `hackpro.json` is user data.
* The save/load JSON schemas and the native Best Run field mapping.
* The rest of the lifecycle: TLS callbacks, `early-load` ordering, the shutdown path.
* `+0x2C4` and `+0x35C` as GJGameLevel offsets.
* The serialiser's `this` base in `notes/settings-registry.md`.
* The exact C++ type of the registry's map container (it is not `std::unordered_map`; what it
  is remains UNVERIFIED).
* Hook-target RVAs still flagged UNVERIFIED per row in `_hook_sites.tsv`.

## What this audit did not do

No M2 work was started. No commits, no pushes. No binary entered the index.
