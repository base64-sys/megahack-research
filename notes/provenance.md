# Provenance and baseline pinning

Closes the first open exit criterion of [M1](../docs/ROADMAP.md#m1--binary-inventory-and-host-abi-surface-in-progress).

- **Question** — which bytes are we actually reading, and can they be re-derived from
  something other than one extracted folder?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`.

## Method

Two independent tools, per the verify-twice rule:

1. `Get-FileHash -Algorithm SHA256` (PowerShell 5.1) over the sample set.
2. Python 3.14.6 `hashlib.sha256`, streamed from inside the `.geode` archive with
   `zipfile`, so the packaged copy is compared against the extracted copy rather than
   against a filename.

Byte-level structural checks used `pefile` 2024.8.26 and `capstone` 5.0.9 from
`samples/absolllute.hackmega/.analysis_tools/`, plus raw slicing of the `.text` section.

Scratch scripts used for this investigation lived outside the repo, in
`%TEMP%\opencode\`. They were used once each and are not promoted to `scripts/`.

## Evidence — digests

| File | Bytes | SHA-256 |
| ---- | ----- | ------- |
| `absolllute.hackmega_original.dll` | 8,697,056 | `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92` |
| `absolllute.hackmega.dll` | 8,697,056 | `24cebcac315c0d3e7ff26a0515970b2fcade98c8f665ebe825d7da0b8aa47996` |
| `absolllute.hackmega_pre_persistence.dll` | 8,697,056 | `3b1167675aece8aae9b5fa01b5c843b68892a88f3e64974631a80aa35fed0061` |
| `absolllute.hackmega_working_aslr.dll` | 8,697,056 | `3b448741f8eafb9a4b6e392cbe07f0872572eb211cc7a37bbae629a884636b6b` |
| `absolllute.megahack.android32.so` | 7,591,352 | `57bbe8d84272eb7c9d3a9c2cfe0dd83544b4531829daaf610a59bfd6c981b913` |
| `absolllute.megahack.android64.so` | 10,721,792 | `06489f3b6ec0ad08ecdd07c0e3dd7b69bff3571c3a6c1fd574a6dae0249e31da` |
| `absolllute.hackmega.geode` | 12,221,406 | `ee62eb39d20f0fddff601b4727a06a40d3e3791ec131545fda260f1013ba99a1` |
| `absolllute.hackmega.geode.zip` | 12,221,406 | `ee62eb39d20f0fddff601b4727a06a40d3e3791ec131545fda260f1013ba99a1` |
| `mod.json` | 941 | `bd1a0274093a972b035ca7829f003f81b8f08cf2b476e9b2169f324b407fc9ea` |
| `changelog.md` | 47,416 | `928f1e7e88417cd1150ffe77c2aaba95df771044e56a4d034a1b8001503e2a59` |
| `about.md` | 1,976 | `75bcef12e43fd1f5b45ff7006bbb6f370e1497ad0b5c381ef8dd1a507b81bc2d` |

All four x64 DLLs are exactly the same size (8,697,056) but have four different digests.
Size therefore carries no information about which build is which — only the digest does.

### Finding: the `.geode` package in `samples/` is NOT pristine

**RESOLVED.** See [Cross-check against a pristine package](#cross-check-against-a-pristine-package)
below. `samples/PRISTINE_MEGAHACK.geode` was supplied later, and `_original.dll` matches it
byte for byte. The findings below still stand for the *modified* package.

The `.geode` archive is byte-identical to `.geode.zip` (a Geode package is a zip), and the
`absolllute.hackmega.dll` **inside** it hashes to:

```
24cebcac315c0d3e7ff26a0515970b2fcade98c8f665ebe825d7da0b8aa47996
```

which is exactly the digest of the extracted `absolllute.hackmega.dll`, i.e. the *generated
extended persistence build*. It is **not** the digest of `_original.dll`.

This is consistent with `tools/update_geode_package.py` having written the patched DLL back
into the archive, as its own docstring states. The package also carries `Edit Best Runs.cmd`,
`i.ps1`, `tools/best_runs_db.py`, `tools/best_runs_editor.py` and
`tools/import_dt_setting.ps1` — helpers from the persistence patch, not part of an
untouched release.

Consequences:

- ~~The roadmap's M1 criterion "cross-check `_original.dll` against a second extraction of
  `absolllute.hackmega.geode`" **cannot be satisfied from the current sample set**.~~
  **Superseded** — see below. The check was performed against `PRISTINE_MEGAHACK.geode`
  and passed.
- The only pristine x64 artifact in the sample set is `_original.dll`. It must be preserved
  bit-for-bit; nothing else can stand in for it.
- The Android `.so` files *are* pristine: their digests inside the package match the
  extracted copies exactly (`57bbe8d8…`, `06489f3b…`). They are usable as-is for M5.

## Cross-check against a pristine package

`samples/PRISTINE_MEGAHACK.geode` (12,526,065 bytes, SHA-256
`b08e879e45ce555666da5c1048413e3131f687bee02147bf121b4855c0133dbf`, 63 entries) was supplied
by the user on 2026-10-17 after the question was raised.

Every relevant entry was extracted and hashed:

| Package entry | SHA-256 | Matches |
| ------------- | ------- | ------- |
| `absolllute.hackmega.dll` | `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92` | **`_original.dll`, exact** |
| `absolllute.megahack.android32.so` | `57bbe8d84272eb7c9d3a9c2cfe0dd83544b4531829daaf610a59bfd6c981b913` | extracted copy, exact |
| `absolllute.megahack.android64.so` | `06489f3b6ec0ad08ecdd07c0e3dd7b69bff3571c3a6c1fd574a6dae0249e31da` | extracted copy, exact |
| `about.md` | `75bcef12e43fd1f5b45ff7006bbb6f370e1497ad0b5c381ef8dd1a507b81bc2d` | extracted copy, exact |
| `changelog.md` | `928f1e7e88417cd1150ffe77c2aaba95df771044e56a4d034a1b8001503e2a59` | extracted copy, exact |
| `mod.json` | `7626bcaed75cddf399dd04d4f36851091eff3594b83e2afe12520cc7a37cfcfb` | **differs** |

**The cross-check passes.** The DLL shipped in a pristine v9.1.3 package is bit-identical to
`absolllute.hackmega_original.dll`. `_original.dll` is now independently verified, and the
M1 criterion that was blocked is met after all.

Two secondary observations:

- `mod.json` differs by one byte (940 shipped vs 941 extracted) with **no content change** —
  `difflib` reports no differing lines, so the patch tooling rewrote the file with different
  line endings or a trailing byte. Nothing semantic was altered.
- The extracted folder is a **superset** of the pristine package: no entry present in the
  pristine archive is missing from `samples/absolllute.hackmega/`. The extras are the
  persistence-patch helpers. So the folder is the pristine release plus a known, enumerated
  delta, which is the best possible arrangement for this project.

## Evidence — code cave

Cave at VA `0x180286371`, RVA `0x286371`, file offset `0x285771`, size `0xc8f` (3,215 bytes),
inside `.text`.

| File | Cave uniform | First byte |
| ---- | ------------ | ---------- |
| `_original.dll` | yes | `0xCC` |
| `_working_aslr.dll` | no | `0x48` |
| `_pre_persistence.dll` | no | `0x48` |
| generated `.dll` | no | `0x48` |

The cave is all `0xCC` only in `_original.dll`, which is direct byte-level evidence that
the ground truth is unpatched and that all three experiments write into this region.

## Evidence — mutable state block

Claim under test: 48 zero-initialised bytes at preferred VA `0x18081BFC0`.

A first attempt read file offset `0x81B1C0` and reported 35 non-zero bytes, which would
have contradicted the claim. **That reading was invalid.** `0x81B1C0` is inside `.pdata`
raw data `[0x7ff800, 0x839400)` — unwind information, a completely different location. The
RVA→file mapping is not linear once an RVA falls past a section's `SizeOfRawData`.

Corrected analysis, using `pefile`:

| Quantity | Value |
| -------- | ----- |
| State RVA | `0x81BFC0` |
| `.data` VirtualAddress | `0x6DF000` |
| `.data` VirtualSize | `0x13CD90` |
| `.data` virtual end | `0x81BD90` |
| `.data` SizeOfRawData | `0x121600` |
| `.data` raw data file range | `[0x6DE200, 0x7FF800)` |
| `SectionAlignment` | `0x1000` |
| `.data` page-rounded mapping end | `0x81C000` |
| Containing page RVA | `0x81B000`–`0x81BFFF` |
| `.data` characteristics | `0xC0000040` = `CNT_INITIALIZED_DATA \| MEM_READ \| MEM_WRITE` |

So `0x81BFC0` sits:

- **beyond** `.data`'s `VirtualSize` end (`0x81BD90`), in the 0x230-byte inter-section gap
  before `.pdata` starts at `0x81C000`;
- **beyond** the raw data end, so **no file byte corresponds to it** — the loader must map
  it as zero-fill;
- **inside page `0x81B000`**, which `.data` commits and protects read/write, because the
  loader rounds `VirtualSize` up to `SectionAlignment` (`0x13CD90` → `0x13D000`, mapping end
  `0x81C000`).

The block is therefore writable and zero at load time, as claimed. The RE notes state the
conclusion correctly but not the reason, and the reason is the only fragile part: the block
is not *inside* `.data` by the letter of `VirtualSize`. It is inside `.data` by the letter of
page rounding. Any future change to `VirtualSize` that shrinks the page-rounded end below
`0x81C000` would silently make this state block unmapped.

UNVERIFIED: that the block is genuinely writable and zero at runtime. Confirming this needs
a live load of `_original.dll` under a debugger and a read of `[0x18081BFC0]`. What is
verified is the static mapping arithmetic above.

## Evidence — patch sites, cross-checked against the RE notes

Byte-level diff of each experiment against `_original.dll` across the whole of `.text`
(file `0x400`..`0x533400`, VA `0x180001000`+).

| Variant | `.text` bytes changed | In cave | Outside cave | Out-of-cave sites |
| ------- | --------------------- | ------- | ------------ | ----------------- |
| `_working_aslr` | 624 | 611 | 13 | 3 |
| `_pre_persistence` | 171 | 166 | 5 | 1 |
| generated | 3,260 | 3,214 | 46 | 12 |

Union of all out-of-cave patched byte addresses: **48**. Every one of the 48 falls within
±8 bytes of one of the 13 hook addresses documented in
`samples/absolllute.hackmega/RE_NOTES_PERSISTENT_BEST_RUN.md`. Coverage 48/48, so the hook
map is fully corroborated from the baseline binary rather than taken on trust.

Out-of-cave patch ranges per variant, as observed:

| Variant | Ranges |
| ------- | ------ |
| `_working_aslr` | `0x1801552D7`–`0x1801552D9`, `0x1801553EC`–`0x1801553F2`, `0x180155409`–`0x18015540B` |
| `_pre_persistence` | `0x1801552D7`–`0x1801552DE` |
| generated | `0x1800023DD`–`0x1800023DF`, `0x180027C30`–`0x180027C34`, `0x180098869`–`0x18009886B`, `0x180155155`–`0x18015515B`, `0x1801552D7`–`0x1801552D9`, `0x1801553EC`–`0x1801553F2`, `0x180155409`–`0x18015540B`, `0x180155419`–`0x180155420`, `0x18015546C`–`0x18015546E`, `0x18015558C`, `0x1801555AB`, `0x180174DE5`–`0x180174DE7` |

Observation, not yet explained: at six hooks the patched range begins one byte *after* the
documented address (`0x1801552D6`→`…2D7`, `0x180155408`→`…409`, `0x18015546B`→`…46C`,
`0x180174DE4`→`…4E5`, `0x1800023DC`→`…3DD`, `0x180098868`→`…869`), i.e. the leading opcode
byte is preserved and only the operand is rewritten. At four hooks (`0x180155155`,
`0x1801553EC`, `0x180155419`, `0x180027C30`) the first byte does change.

(Resolved 2026-10-20.) At `0x1801552D6` the first instruction is `call 0x1801A4F30`
(`e8 55 fc 04 00`); the patched range `0x1801552D7`–`0x1801552D9` rewrites only the
rel32 operand, leaving the `e8` opcode. At `0x180155155` the first instruction is
`mov rcx, [rip+...]` and the patched range starts at the same address, so the opcode byte
is rewritten. The two groups differ by whether the hook is installed by patching a call's
target (operand only) or by replacing the instruction outright (opcode included).

## Conclusion

1. `_original.dll` (`1635c339…`) is the pristine v9.1.3 release DLL, now **independently
   verified** against `PRISTINE_MEGAHACK.geode`, and confirmed unpatched by its
   uniformly-`0xCC` cave.
2. The four same-sized DLLs must be told apart by digest, never by size or filename.
3. The `.geode` package in `samples/` has been modified and cannot serve as a second
   extraction, but `PRISTINE_MEGAHACK.geode` supplies that check and it passes.
4. The 13 documented hook addresses are corroborated: 48/48 out-of-cave patched bytes are
   accounted for.
5. The mutable-state block at `0x18081BFC0` is zero-filled and writable, but by page-rounding
   rather than by `VirtualSize`. Record the fragility.

## Open questions

- ~~Obtain a pristine v9.1.3 `.geode` and re-run the cross-check.~~ **Done 2026-10-17** —
  `PRISTINE_MEGAHACK.geode` supplied, cross-check passed.
- (Resolved 2026-10-20.) `compare_platform_classes.py` extracts 474 MSVC class names from
  `_original.dll` and 1,108/1,110 Itanium names from the Android `.so` files, with zero
  overlap by naive name extraction because the ABIs encode type names differently (MSVC
  `.?AVName@@` vs Itanium `_ZN...`). The Android builds carry no MSVC RTTI, so a shared-
  code check needs M5's separate pass, not this script. UNVERIFIED: whether the two
  platforms ship identical feature code.
- (Resolved 2026-10-20.) The opcode/operand question is resolved: some patched ranges start
  on the opcode byte, others one byte later on the operand, depending on whether the hook is
  installed by replacing the whole instruction or by rewriting a call target. See the
  disassembly in the section above.
- (Resolved 2026-10-20.) `build_persistence_patch.py`, `validate_persistence_patch.py`,
  `inspect_interface_map.py`, `pe_*.py` and the three Ghidra scripts are absent from the
  `.geode` package, so they are original to this project rather than Absolllute's and may be
  promoted into `scripts/`. The `.geode`'s own `tools/` (best_runs_db.py etc.) are
  Absolllute's and stay in `samples/`. UNVERIFIED: whether any promoted script depends on
  a sample-only helper.

## Failure modes recorded

- Reading a byte at RVA `0x81BFC0` by computing `PointerToRawData + (rva - VirtualAddress)`
  when `rva - VirtualAddress > SizeOfRawData` silently returns bytes from a *later section*.
  Always range-check the RVA against `VirtualSize` and `SizeOfRawData` before converting.
- A stale loop variable in a throwaway script printed `.reloc`'s section characteristics
  while labelling the output `.data`. Section characteristics in these notes were re-read
  directly and are `0xC0000040` (read/write) for `.data`.