# Best Run feature cluster

- **Question** — which code and settings implement Mega Hack's Best Run features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

| Setting key | Observed value | Role |
| --- | --- | --- |
| `STATUS/BEST_RUN/DROPDOWN/ACCURATE_PERCENTAGE` | False | Enables accurate percentage in Best Run |
| `STATUS/BEST_RUN/DROPDOWN/ONLY_CURRENT` | True | Only show current StartPos runs |
| `STATUS/BEST_RUN/DROPDOWN/PREFIX` | True | Reused row for native Import .dt |
| `COSMETIC/HACK_NO_NEW_BEST_POPUP` | (listed) | Suppresses the New Best popup |

## Evidence — New Best label

From [new-best-label.md](new-best-label.md):

- `PlayLayer::showNewBest` is hooked at `0x180169DE0` by `FUN_180169EB0`
  (`0x180169EB0`), with `PlayLayer::destroyPlayer` hooked at `0x180169F20`.
- Setting `245` (`0xF5`) suppresses the whole path.
- Language key `0052` selects opacity `0.5`; otherwise opacity `1.0`.
- Settings `247` (`0xF7`), `501` (`0x1F5`) and `502` (`0x1F6`) gate the
  opacity scale inside `FUN_18017F000`.

## Evidence — native storage

From [persistence.md](persistence.md) and [save-data-hooks.md](save-data-hooks.md):

- Mega Hack stores no Best Run data itself.
- Its only storage-side hook is `GJGameLevel::savePercentage`
  (`0x180199D10` / callback `0x180199DF0`), which can veto the call but never
  write.
- Best Run is GD's own saved percentage.

## Evidence — native Import .dt

From [import-dt.md](import-dt.md):

- The native action reuses the former Show Prefix row.
- Its tag is resolved after core load and intercepted by the boolean setter.
- The file picker is `FUN_1800C6300` wrapping `GetOpenFileNameW`.
- Cancelling the picker makes no changes.

## Open questions

- UNVERIFIED: the exact GD-side fields carrying
  `(level, startPercent) -> bestEndPercent`. They are not recoverable from
  Mega Hack's binary.
- UNVERIFIED: which settings keys correspond to tags 247, 501 and 502.
  245 is the Map B hack-flag tag `HACK_RESPAWN_TIME` (static label map plus the
  `isHackEnabled` getter path); its runtime effect on the label is still to confirm. See
  [static-hack-label-map.md](static-hack-label-map.md).

## Evidence - Better Calculation (tag 0x1AF) replaces getCurrentPercent (2026-10-28)

The v9.1.0-beta.5 row "Added Accurate Percentage Better Calculation (fixes StartPos!)" is
the third tag in the percentage family, and it does not format anything: it replaces the
**value**.

**Method.** `build_xref_index.py --grep` over the recovered tree for the percentage strings,
then a whole-`.text` capstone scan for the tag immediate, then instruction bytes with
`samples/absolllute.hackmega/tools/pe_disasm.py`.

**Boundary for "read once".** A linear capstone disassembly of the entire `.text` section
(RVA `0x1000`-`0x533FE6`) reports exactly one instruction whose immediate operand is
`0x1AF`:

```
0x1801655ED  ba af 01 00 00   mov  edx, 0x1af
```

That is inside `FUN_1801655E0` (RVA `0x1655E0`, file `0x1649E0`), the detour that
`FUN_180165040` installs on `PlayLayer::getCurrentPercent` (RVA `0x3B3950`) when tag 429 is
enabled ([hook-registrations.md](hook-registrations.md), line 159). The search covers every
byte of the section, so this is a bound on the shipped image, not on the recovered subset.

**The replacement formula** (`FUN_1801655E0`, instruction bytes re-read for each operand):

```
0x1801655FE  mov  rax, qword ptr [rsi + 0x878]
0x180165605  mov  eax, dword ptr [rax + 0x4C8]      ; int, tested for zero
0x18016560F  movsd xmm0, qword ptr [rsi + 0x3D0]    ; double
0x180165617  mulsd xmm0, [0x18054C678]              ; 240.0
0x18016561F  cvtsi2sd xmm1, eax
0x180165623  divsd xmm0, xmm1
0x180165627  mulsd xmm0, [0x180534330]              ; 100.0
0x18016562F  xorps xmm1,xmm1 ; cvtsd2ss ; xorps xmm2,xmm2 ; maxss xmm2,xmm1 ...
```

so the returned percentage is `clamp([PlayLayer+0x3D0] * 240.0 / [[PlayLayer+0x878]+0x4C8]
* 100.0, 0.0, 100.0)`, and the fallback path tail-calls `FUN_1801A4CB0`, the lazy trampoline
for host RVA `0x3B3950`, i.e. the game's own value.

**Conclusion.** Tag `0x1AF` (431) selects a distance-over-level-length percentage that never
looks at start position, while 429 stays the master enable and 430 the decimal count used by
`FUN_180165110`. That division of labour matches the three changelog bullets (Accurate
Percentage, Decimal Places, Better Calculation) and fixes this row's old open question: the
formatter and the calculator are separate tags.

UNVERIFIED: that `[PlayLayer+0x3D0]` is a distance in level units and `[GJGameLevel+0x4C8]`
is the level length; the widths are certain (`double` / `int`) but the meanings are inferred
from the 240.0 and 100.0 constants. Closing this needs a GD 2.2081 field anchor or a runtime
read of both offsets during a level.
- UNVERIFIED: that the tag-431 binding is this changelog row rather than an unlisted
  percentage variant. It rests on elimination (429 formatter, 430 decimals, 431 value) and on
  the changelog's "Better Calculation" wording, not on a settings key; no static `HACK_*`
  label exists for 431.

## Evidence — Accurate Percentage (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180165040` (`0x180165040`) registers `FUN_180165110` (`0x180165110`)
as a hook on `PlayLayer::updateProgressbar` (RVA `0x3B23C0`).

`FUN_180165110` formats the progress bar percentage:
- Reads decimal places from tag 0x1AE (430) via `FUN_18002E4C0` (uint/int accessor).
- If decimal places == 0 OR `isHackEnabled(tag 0x1AD = 429)` is true: formats
  with the configured decimal places using `"{{:.{}f}}%"`.
- Computes the percentage from `FUN_1801A4CB0(param_1)` (a position/percentage
  value), applies `pow(10, decimals)` rounding, and clamps to [0, 100].

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1AD | 429 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | "Accurate Percentage" toggle |
| 0x1AE | 430 | `FUN_18002E4C0` (uint/int accessor) | (not in static list) | Decimal places (0–N)

UNVERIFIED: which JSON keys bind to tags 0x1AD and 0x1AE. The second half of this
question is now answered: this formatter is "Accurate Percentage" (tag 429) and the
"Better Calculation" row is the separate tag 0x1AF, see
[Evidence - Better Calculation](#evidence---better-calculation-tag-0x1af-replaces-getcurrentpercent-2026-10-28).
- UNVERIFIED: whether `STATUS/BEST_RUN/DROPDOWN/PREFIX` is the reused row's
  persisted display state or a separate setting.

## Evidence - decimal places (tag 0x1AE) is one integer shared by two formatters (2026-10-28)

The v7.0 row "Added Accurate Percentage Decimal Places" binds to tag `0x430` (`0x1AE`), the
only tag in the percentage family that is read as an integer count.

**Method.** A linear capstone scan of the whole `.text` section (RVA `0x1000`-`0x533FE6`,
`CS_ARCH_X86`/`CS_MODE_64`, matching operand type `X86_OP_IMM`) for immediate `0x1AE` returns
exactly three instructions, so the read boundary is the section and not a guess:

| Site | Containing function | Role |
| --- | --- | --- |
| `0x180165174` `mov edx, 0x1ae` | `FUN_180165110` (`.pdata` `0x180165110..0x180165459`, RVA `0x165110`) | the `PlayLayer::updateProgressbar` detour (tag 429 callback) |
| `0x18024F159` `mov edx, 0x1ae` | `FUN_18024E8D0` (`.pdata` `0x18024E8D0..0x18024FB1D`, RVA `0x24E8D0`, +`0x889`) | the status-label formatter, percentage branch |
| `0x18024F2C1` `mov edx, 0x1ae` | same function, +`0x9F1` | the status-label formatter, second (tag `0x303`) branch |

**Progress-bar formatter** (`FUN_180165110`): the value is read with
`FUN_18002E4C0(registry, 0x1AE)`; `decimals == 2` returns immediately, i.e. two decimal
places is the untouched default. Otherwise `pow(10.0, decimals)` scales the value from
`FUN_1801A4CB0(PlayLayer)` (the lazy trampoline to `getCurrentPercent`, host RVA `0x3B3950`),
`floor` rounds, the result is clamped to `[0.0, 100.0]` (`0x42C80000`), and the format string
is built with `iVar7` as the precision. `FUN_18008F420(registry, 0x1AD)` — tag 429 — selects
the fixed-precision branch when it is set.

**Status-label formatter** (`FUN_18024E8D0`, which also owns `"Best Run: {}"`, `"Attempt {}"`,
`"Frame {}"`, `"{} Deaths"`, `"{} Jumps"`, `"Recording: {}"`, `"Playing: {}/{}"`,
`"Accuracy: {:.2f}%"`, `"Alive"`/`"Dead"` and the position pair `"({:.2f}, {:.2f})"`): both
sites do `pow(10.0, decimals)`, `floor(value * scale)` and then format through the runtime
pattern `"{{:.{}f}}%"` (`local_2b8`/`local_388` = 10 bytes, i.e. `{:.Nf}%`). The second site
additionally requires `FUN_18002DE50(registry, 0x303)`.

**Conclusion.** One integer setting — tag 430 — sets the decimal count for both percentage
renderers, and the progress bar treats `2` as "leave vanilla alone". That is the mechanism of
the v7.0 "Decimal Places" row; the same tag is the one the mapped v9.0.0 Accurate Percentage
row already cites as "configurable decimals".

**Open questions.**

- UNVERIFIED: which settings row writes tag 430. Only reads appear at the three sites above;
  the row's `define`/`setConfigVar` path is not in this note, so the label bound to `0x1AE` is
  inferred from the reads and from the changelog wording.
- UNVERIFIED: what tag `0x303` (771) gates at `0x18024F2C1`. It is a boolean read
  (`FUN_18002DE50`) in the status-label path and has no entry in the mapped rows here.
- The `else` branch of `FUN_180165110` (taken when decimals is not 2 and tag 429 is clear) was
  not read past the branch, so "what happens when Accurate Percentage is off" is unrecorded.
