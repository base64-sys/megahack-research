# Persistence: what Mega Hack v9.1.3 actually writes

- **Question** — which on-disk stores does the release binary use, and which of the
  persistence artefacts in `samples/` belong to it?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-18.
- **Tools** — a raw byte scan of all four Windows images, `_xrefs.tsv`,
  `pe_disasm.py`, `scripts/pe_rva.py`.

## The correction this note exists for

`samples/absolllute.hackmega/RE_NOTES_PERSISTENT_BEST_RUN.md` documents a `best_runs.bin`
sidecar: a 32-byte header (`char[8] magic = "MHBRDB1\0"`, version, header size, record size,
record count, FNV-1a-64 checksum) followed by 24-byte records of
`uint64 levelKey; double startPercent; double bestEndPercent;`.

**That format is not Mega Hack's.** It is the format invented by the local persistence patch
in this repo's `samples/` workspace. The evidence is a byte scan of all four Windows images:

| Byte string | `_original` | `_pre_persistence` | `_working_aslr` | generated `.dll` |
| ----------- | ----------: | ----------------: | -------------: | ---------------: |
| `best_runs` | — | — | 1 | 1 |
| `best_runs.bin` | — | — | 1 | 1 |
| `MHBRDB1` | — | — | — | 2 |
| `Octavus` | 1 | 1 | 1 | 1 |
| `hackpro.json` | 1 | 1 | 1 | 1 |
| `data.json` | 1 | 1 | 1 | 1 |
| `save.json` | 1 | 1 | 1 | 1 |

Two independent angles agree: the byte scan, and `_xrefs.tsv`, in which `best_runs` and
`MHBRDB1` return **zero** referencing functions while `Octavus` returns one.

So the sidecar appears only in binaries produced by `build_persistence_patch.py`. Quoting it
as Mega Hack's format would be exactly the error `AGENTS.md` names: quoting an experiment as
evidence about the release. `docs/ROADMAP.md` did so, and this note corrects it rather than
adding a second version of the truth.

This does **not** make the sample note worthless — its geometry, hook map and the two
recorded crash modes (the mutable-`.text` build failing at `DLL+0x28638E`, the
absolute-preferred-base `.data` build failing under ASLR) are about *patching* Mega Hack and
remain accurate as such. It simply is not evidence about what v9.1.3 does.

## What the release does use

Three stores, all confirmed present in `_original.dll`.

### 1. `hackpro.json` — the mod's own configuration

In the mod directory. Read and written by a matched pair, each 302 bytes:

| Direction | VA | RVA | Shape |
| --------- | -- | --- | -------- |
| read | `0x1800BF190` | `0xBF190` | `std::ifstream`, refcount object `0x120` bytes, open mode `1` |
| write | `0x1800BF400` | `0xBF400` | `std::ofstream`, refcount object `0x118` bytes, open mode `2` |

Both build the path with `FUN_1800BEA70`/`FUN_1800BEAA0` and append via `FUN_1800BF320`.
Both have **zero direct callers**, so `_callers.tsv` has no row for them — a concrete case
for the rule that a missing caller row means zero callers, not dead code.

Contents are parsed by `FUN_18008B780`; see
[settings-registry.md](settings-registry.md#the-insert-and-where-names-come-from). In short,
each JSON member's key is a setting name and its `"tag"` number is a 64-bit `hackpro::Tag`,
with optional `"default"`, `"action"` and `"nosave"`.

### 2. GD's `data.json` and `save.json` — the game save files

`FUN_180042FB0`, VA `0x180042FB0`, RVA `0x42FB0`, 1,587 bytes, one caller. It resolves a base
directory, appends the literal `Octavus` (VA `0x18053FBA5`, the only reference), then reads
`data.json` into its second argument and `save.json` into its third, both with the same
open mode the `hackpro.json` reader uses.

The base-directory resolver is `FUN_1800C5210`, VA `0x1800C5210`, 98 bytes:

```
iVar1 = SHGetKnownFolderPath(&DAT_1805820e0, 0, 0, &local_10);
```

The 16 bytes at VA `0x1805820E0` (RVA `0x5820E0`) read back as
`f1b32785-6fba-4fcf-9d55-7b8e7f157091`, which is **`FOLDERID_LocalAppData`**
(`%LOCALAPPDATA%`). So the chain is `%LOCALAPPDATA%\…\Octavus\{data.json,save.json}`.

Failure path: an empty base directory logs the language key `ERROR/HOME/NO_APPDATA`
(VA `0x18053FB8F`).

UNVERIFIED: the directory segment between `%LOCALAPPDATA%` and `Octavus`.
`_xrefs.tsv` contains **no** reference to the literal `GeometryDash` anywhere in the binary,
so that segment is not a hard-coded string here; it is either supplied by Geode's
directory abstraction or derived at runtime. Confirming it needs the Geode host contract for
save-data paths — see [references/](../references/).

### 3. Nothing else found

`pe_xrefs.py` over the string inventory turned up no other persistence filename in the
release. Absence of a filename is not proof of absence of persistence, so this is a bounded
result: **the three above are every store this pass found**, not an exhaustive proof.

## Where Best Run is stored: it is not, by Mega Hack

This was the file's biggest open question. It now has an answer, and the answer is a
negative — reached by enumeration rather than by inference.

**Every hook Mega Hack installs.** Hook registration has a fixed call shape, but the
registration helper varies by callback signature -- 143 distinct helpers, all calling
`geode::hook::createConvention` -- so the full sweep in `scripts/hook_sites.py` now yields
**338 registration sites over 215 distinct GD methods** (see
[hook-dispatch.md](hook-dispatch.md#the-complete-hook-surface)). Of those 215, the only
storage-side method is `GJGameLevel::savePercentage`, and its callback `FUN_180199DF0`
cannot write anything -- it either vetoes the call or forwards it:

```
cVar1 = FUN_18019b5f0();
if (cVar1 != '\0') return;                                  // blocked
FUN_1801462c0(param_1, param_2, param_3, param_4);           // the original
```

So **Mega Hack v9.1.3 does not store Best Run data.** Best Run is GD's own saved
percentage, and Mega Hack's role is to stop a cheated run from being persisted by vetoing
`GJGameLevel::savePercentage`. That also explains why the local patch had to invent
`best_runs.bin`: there was no Mega Hack table to extend, because there is no Mega Hack
table.

Two independent supports:

1. **No write path.** A per-level `(level, startPercent) -> bestEndPercent` table would need
   a hook that writes it, or a serializer that emits it. The complete hook list above has
   neither, and `FUN_180076EB0`, the only registry serialiser, writes the five
   `V_*` type groups -- no per-level structure.
2. **No room in the schema.** `hackpro.json` is a flat name -> typed-value map
   (bool / int / double / string / bytes), and the single value slot per name cannot hold a
   map.

UNVERIFIED: the completeness of the 338-site sweep. It is a textual sweep of the recovered
tree for one call shape (a quoted `Class::method` plus a trailing `, <order>, 1)`), so a
registration whose call Ghidra split, inlined or rendered differently would be missed. The
215 method names are what this pass found, not a proof
that no thirteenth exists. Reading the registration helper's own parameters would make it
rigorous.

A consequence worth recording: `_host_methods.tsv` lists **214** methods that Mega Hack
*names*, but only **12** of them are *hooked*. Naming a method — for diagnostics, for a
lookup, or incidentally — is not hooking it, and the two numbers should not be confused.

## Native Best Run fields (GD's schema, not Mega Hack's)

M1/M4 residual work: identify which fields of GD's own structures carry the
`(level, startPercent) -> bestEndPercent` data. Investigated 2026-10-21.

### What Mega Hack actually does with Best Run

Mega Hack v9.1.3 stores **no** Best Run data (established in this note above). Its
only storage-side hook is `GJGameLevel::savePercentage` (registered in
`FUN_180199D10`, VA `0x180199D10`), whose callback `FUN_180199DF0`
(`0x180199DF0`) either vetoes the call (when `FUN_18019B5F0()` is true) or forwards
it to GD's original at RVA `0x16C8B0`:

```
cVar1 = FUN_18019b5f0();
if (cVar1 != '\0') return;          // blocked: GD never learns the percentage
FUN_1801462c0(param_1, param_2, param_3, param_4);   // the original
```

The thunk `FUN_1801462C0` (`0x1801462C0`) truncates three arguments to 32 bits
before forwarding, so the call shape is `savePercentage(GJGameLevel*, int, int, int)`.
The three ints are GD's own (startPercent, endPercent, and a mode/flag are the
plausible readings), but their meaning is GD's, not Mega Hack's.

Mega Hack resolves no other GJGameLevel or PlayLayer method that reads or writes a
"best percentage" field: `_host_methods.tsv` contains only `GJGameLevel::savePercentage`
among GJGameLevel methods, and the PlayLayer methods Mega Hack names are lifecycle/render
hooks (`resetLevel`, `postUpdate`, `levelComplete`, `getCurrentPercent`, …), none of
which is a best-run accessor. `getCurrentPercent` (`FUN_1801655E0`,
`0x1801655E0`) reads `PlayLayer + 0x3D0` (double) and `GJGameLevel + 0x4C8` (int)
to compute the *current* percentage; it is not a best-run field.

### Conclusion: GD's schema is not recoverable from this binary

The `(level, startPercent) -> bestEndPercent` storage lives in **Geometry Dash's own
executable** (`GeometryDash.exe`), which is outside this repository's ground truth
(`_original.dll` is Mega Hack). Mega Hack never reads or writes GD's best-run fields;
it only vetoes `savePercentage`. Therefore:

- No GJGameLevel or PlayLayer offset in Mega Hack's code can be identified as the
  carrier of `bestEndPercent`, because Mega Hack does not touch that carrier.
- The flush points are GD's (`savePercentage` invocation), not Mega Hack's, except
  that Mega Hack's `resetLevel`/`levelComplete` hooks bracket GD's calls.
- The `data.json` field question in M4 is likewise unanswerable here: Mega Hack reads
  `data.json`/`save.json` only as opaque blobs via `FUN_180042FB0` (`0x180042FB0`);
  it does not parse individual best-run entries.

**UNVERIFIED / not recoverable:** the exact field offsets inside GD's structures
that hold `(level, startPercent) -> bestEndPercent`, and which `data.json` keys
carry them. Confirming requires either the GD executable (out of scope/baseline) or
a runtime observation of GD writing best-run data. The honest state for M4 is that
Mega Hack's own role is fully mapped (veto only); GD's internal schema is a
separate, GD-side question and is left as UNVERIFIED with this rationale.

## Open questions

- **Where does v9.1.3 natively persist Best Run data?** **Answered: it does not.** See the
  section above — the complete hook list contains no writer, and the one storage-side hook
  can only veto. Mega Hack reads GD's saved percentage and blocks GD from persisting a
  cheated one.
- (Resolved 2026-10-20.) There is **no** intermediate segment: `FUN_1800C5210` calls
  `SHGetKnownFolderPath` with the `FOLDERID_LocalAppData` GUID at `0x1805820E0`
  (`F1B32785-6FBA-4FCF-9D55-7B8E7F157091`), and `FUN_180042FB0` appends `Octavus`
  directly, then `data.json`. Verified by reading the GUID bytes and the decompiled path
  join. UNVERIFIED: whether `Octavus` is a fixed literal or derived from a publisher name;
  only the literal is visible.
- (Partially resolved 2026-10-20.) `FUN_1800BF190` and `FUN_1800BF400` occur as adjacent
  slots in a vtable-shaped array at `0x180547890` (`+0x20` and `+0x28`), alongside
  `0x1800BEED0`/`0x1800BEF80`/`0x1800BF030`/`0x1800BF0E0`. That explains the zero direct
  callers: they are reached by virtual dispatch, not by name. UNVERIFIED: that this is the
  vtable installed by the registry constructor (`FUN_1800A27E0` writes vftables at object
  `+0x00` and `+0x128`), and on which trigger (timer, shutdown, or both) the writer slot
  is invoked. Confirming the trigger needs a runtime trace or a caller of the virtual slot.
- `FUN_180042FB0` has one caller, `FUN_180042D70`; whether it is a startup-only read or also
  a migration path is untraced.
- `FUN_18019B5F0`, the shared hook gate, is now traced in
  [hook-dispatch.md](hook-dispatch.md#the-shared-gate-fun_18019b5f0) and is the anti-cheat
  predicate; what the `>= 1.0` progress test means is still open.

## Failure modes recorded

- **Promoting an experiment to a specification.** A long, detailed, internally consistent
  document about `best_runs.bin` reads like ground truth, and it is easy to accept it as
  "Mega Hack's format" because it is the only format anyone has written down in detail.
  The check that settles it is one byte scan of the ground-truth image. Run that before
  believing any format claim, including your own.
- `pe_xrefs.py <image> best_runs` did not return within 120 s and the shell terminated the
  command. A substring that does not occur in the binary appears to be the worst case for
  its search. Prefer `_xrefs.tsv` for existence questions about known strings, and a direct
  byte scan when the question is "is this string present at all".
## Native Best Run fields (GD 2.2081, read-only reference)

Source: disassembly of GJGameLevel::savePercentage at RVA 0x16C8B0 in
GeometryDash.exe 2.2081 (base 0x140000000), analysed read-only. Mega Hack never
reads or writes these fields — it only vetoes the call via the savePercentage hook
(RVA 0x16C8B0, registered in FUN_180199D10). The mapping below is GD's own schema.

Fields on the GJGameLevel object (all dword unless noted), offsets relative to
the object base:

| Offset | R/W in savePercentage | Role (inferred) |
| ------ | --------------------- | --------------- |
| +0x308 | W | best-run value (set from r15d; written when a new best is recorded) |
| +0x30C | W | paired value (+0x308-adjacent, written from a time-derived eax) |
| +0x310 | W | (written from r15d) |
| +0x314 | W | (written from eax + r15d) |
| +0x318 | W | (written from a time-derived value) |
| +0x31C | W | (written from ebx) |
| +0x320 | W | (written, a composite hash-like value) |
| +0x324 | W (byte) | (written from a byte) |
| +0x328 | W | best end percent (normal mode) — written edi (clamped 0..100) on a new best |
| +0x32C | W | time-derived value (+0x328-adjacent) |
| +0x330 | W | +0x32C + edi |
| +0x334 | R/W | (read at 0x14016CB5C, written at 0x14016CB93) |
| +0x338 | R/W | (read at 0x14016CB62, written at 0x14016CB8B) |
| +0x33C | W | (written edi) |
| +0x340 | R/W | (read at 0x14016C98B, written at 0x14016C9C6) |
| +0x344 | R/W | (read at 0x14016C991, written at 0x14016C9BE) |
| +0x348 | W | (written edi) |
| +0x34C | R/W | best value in practice mode (read at 0x14016C904, written at 0x14016C953) |
| +0x4A8 | (string) | a std::string/record (lea rbp,[rsi+0x4a8], passed to a string-append helper) |

Two parallel tracks are visible: +0x308..+0x330 and +0x334..+0x34C, selected by the
practice-mode flag (r8b) and by the ebp < edi comparison at 0x14016C997. The exact
semantic labels (which field is "best end percent" vs. "best start percent" vs. a time
value) are inferred from write patterns and remain UNVERIFIED against GD's own
serialization; a second GD-side function touching the same fields was not identified
(levelComplete at 0x3A7A80 and resetLevel at 0x3B8EB0 touch different offsets).

Mega Hack's own documented GJGameLevel offsets (+0x148, +0x14C, +0x158, +0x198,
+0x1B8) are different fields and are verified in Mega Hack's binary. +0x2C4 and +0x35C
are not among the Best Run fields and, per the machine-code sweep in
gd-structures.md, are not accessed on a GJGameLevel in Mega Hack's binary either — they
live on a different GD structure (the host object at RVA 0x17B4A0).
