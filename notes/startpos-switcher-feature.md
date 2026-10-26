# StartPos Switcher feature cluster

- **Question** — which settings and code implement Mega Hack's StartPos
  Switcher?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/ALLOW_RATED`
- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/DISABLE_IN_PRACTICE`
- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/HIDE_INTERFACE`
- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/QUICK_SWITCH/TOGGLE`
- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/SWITCH_AFTER_DEATH`
- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/L` (byte shortcut)
- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/R` (byte shortcut)
- `LEVEL/STARTPOS_SWITCHER/DROPDOWN/QUICK_SWITCH/SHORTCUTS/0..15`

## Evidence — implementation

- `mega::hacks::startpos_switcher::Interface` is constructed by
  `FUN_180184710` (`0x180184710`).
- `GJBaseGameLayer::loadStartPosObject` is hooked at `0x180172B90`
  (callback `0x180172C00`, helper `FUN_180172C30`).
- The UI label `HACK_STARTPOS_SWITCHER` is registered in
  `FUN_180062940` (`0x180062940`).

## Changelog features

- "Added StartPos Switcher quick switch"
- "Added StartPos Switcher Quick Switch toggle"
- "Fixed StartPos Quick Switch showing on Android"

## Open questions

- UNVERIFIED: which functions read each StartPos Switcher setting tag.
- UNVERIFIED: whether quick-switch shortcuts are global or per-level.
- UNVERIFIED: how `loadStartPosObject` is modified (observe, modify, or veto).
- UNVERIFIED: whether the Android build omits the feature entirely or only the
  quick-switch UI.

## Evidence — StartPos Switcher gate (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180185710` (`0x180185710`) is the gate function. Returns 0 (allow switch)
or 1 (block switch):

1. If `isHackEnabled(HACK_STARTPOS_SWITCHER = 130, 0x82)` is false → returns
   `FUN_18015A2F0() ^ 1` (a level-state check).
2. If `getConfigVar<bool>(tag 0xA6 = 166)` is false (not "only current StartPos"):
   adjusts the StartPos index fields at `PlayLayer+0x878` (+0x388, +0x38C, +0x390).
3. If `getConfigVar<bool>(tag 0xA7 = 167)` is true ("switch after death") AND
   `PlayLayer+0x31F0 != 0`: returns `FUN_18015A2F0() ^ 1`.
4. Otherwise (default): returns 0 (allow).

Callers: `FUN_1801857F0` (switches to next StartPos) and `FUN_180185910`
(switches to previous), both gated by this function. `FUN_180184BA0` is a
larger function that also calls the gate.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x82 | 130 | `isHackEnabled` (Map B) | `HACK_STARTPOS_SWITCHER` | Master toggle |
| 0xA6 | 166 | `getConfigVar<bool>` (Map A) | (not in static list — runtime-assigned) | "Only current StartPos" |
| 0xA7 | 167 | `getConfigVar<bool>` (Map A) | (not in static list — runtime-assigned) | "Switch after death"

UNVERIFIED: which JSON keys bind to tags 0xA6 and 0xA7; the semantics of
`PlayLayer+0x31F0` and `FUN_18015A2F0`.

## Evidence — StartPos Switcher family (2026-10-26)

Read from `absolllute.hackmega_original.dll` with the tracked `pe_rva` / `func_bounds` /
`import_names` helpers and capstone; RVAs below are file-backed and quoted only after a
`.pdata` or instruction-boundary check.

### Correction (2026-10-26): the two old tag rows are withdrawn

The tag map above is kept for history but its two sub-tag rows are **withdrawn**:

| Old claim | Status | Replacement |
| --- | --- | --- |
| `0xA6` (166) = "Only current StartPos" | withdrawn as a binding | by elimination it is `ALLOW_RATED` (UNVERIFIED, no direct read) |
| `0xA7` (167) = "Switch after death" | **wrong** | `0xA8` (168) is switch-after-death; `0xA7` (167) is `DISABLE_IN_PRACTICE` |

Why: the deferral gate inside `0x180185CE0` reads tag 168 (`0x18002DE50`) and, when true,
parks the choice in `0x18080C878` + flag `0x18080C880` for the next reset — that is
switch-after-death, and the static label map has entry 17 `CHECKBOX_SWITCH_AFTER_DEATH`
(tag 168, store `0x180065055`). Tag 167 sits behind `byte [layer+0x31F0] != 0`, the same
"practice mode" byte used by Replay Last Checkpoint and Collect Coins in Practice, which
matches `DISABLE_IN_PRACTICE`; tag 169 has static map entry 4 `CHECKBOX_HIDE_INTERFACE`
(store `0x180064B32`) and is read by the interface show/hide function itself.

### Hooks installed for tag 130

Registration `0x1800B2E30`, callback `0x180184920` (three installers in one body):

| Target | RVA | Detour | Installer | Name string |
| --- | --- | --- | --- | --- |
| `PlayLayer::resetLevel` | `0x3B8EB0` | `0x180184A80` | `0x1800AB850` | `0x180545515` |
| `UILayer::handleKeypress` | `0x4CD830` | `0x180184B10` | `0x1801742E0` | `0x180576044` |
| `PauseLayer::keyDown` | `0x37ED10` | `0x180184B60` | `0x180185EA0` | `0x180578CD0` |

All three are installed with order `0xFFFFFFE0` and enable byte `[rdx]`; the disable branch
resolves through `0x180108A80`.

### Key dispatch (`0x180185070`, called by both key hooks)

1. `0x18015AFD0(layer, &vec, 0)` reads group 0 of the `absolllute.megahack` user dictionary
   (`CCNode::getUserObject` / `CCDictionary` — the same helper Smart StartPos uses); empty
   list returns immediately.
2. Keybind tags **131** (`0x83`) and **132** (`0x84`) are read with `0x18002EE40` and
   compared with the pressed key; a match calls `0x1801857F0(layer)` (next) or
   `0x180185910(layer)` (previous).
3. Otherwise, boolean tag **149** (`0x95`, quick switch) gates a 17-entry shortcut loop over
   the table at `0x1805793A8`: `0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0x96,
   0xA0,0xA1,0xA2,0xA3,0xA4,0xA5` (tags 150-165, with `0x96` repeated at index 10). A
   matching key reloads the list and calls `0x180185CE0(layer, list[index - 1])`, so index 0
   selects "no startpos".

The pause hook resolves the live play layer through `0x1801ACDB0` first, so the shortcuts
work from the pause menu as well as from `UILayer`.

### Gate `0x180185710`

Order of checks: empty list → block; `isHackEnabled(130)` (`0x18008F420`) false → block;
tag **166** true skips the rating check, otherwise the delta `[layer+0x878]+0x388` minus
`+0x38C` is written to `+0x390` and a positive value blocks; tag **167** true plus
`byte [layer+0x31F0] != 0` blocks; otherwise `0x18015A2F0(layer) ^ 1`.

### Next / previous and apply

`0x1801857F0` / `0x180185910` scan the group-0 list for the current startpos
(`[layer+0x3188]`, or the pending `0x18080C878` while a switch is deferred), then pick the
neighbouring entry (wrapping; "not found" means the last entry). They also refresh the
pointer indicator: `[layer+0x3540]` → `0x180185B50` → `0x180185A20(node, 1)` for next and
`(node, 0)` for previous.

`0x180185CE0(layer, startpos)` then either defers (tag 168, see the correction) or applies
immediately: `0x180158F50`, optional `[layer vtable+0x568]` when `CCArray::count(
[layer+0x3888]) != 0`, clears `[layer+0x3880]`/`[layer+0x3188]`, sets
`[layer+0x3230] = (startpos != 0)`, calls `0x180129AE0(layer, startpos)`, `[layer vtable+
0x580]`, and finally refreshes the interface (`0x1801ACDB0` → `[playlayer+0x3540]` →
`0x180185B50` → `0x180184BA0(node, 0)`).

The deferred path is consumed by the reset hook `0x180184A80`: when `[0x18080C880] == 1`
it reads the pending object, applies the same clear/assign sequence, calls
`0x180129AE0(layer, pending)`, and clears the flag; it then calls the cached original
`resetLevel` (`0x1801A4300`, host RVA `0x3B8EB0`) and tail-jumps `0x1801A5D30` when
`[layer+0x3188]` is set, otherwise returns. `0x1801846C0` (the `PlayLayer::init` detour)
clears `0x18080C880` so a pending switch never crosses level loads.

### The interface is installed unconditionally

`0x180184490` (.pdata extent `0x180184490..0x180184639`, 0x1A9 bytes) is **not** a tag
callback: it is called directly from the shared initialiser at `0x1800B2DEE`, between the
tag-235 and tag-130 registrations, so it runs on every start regardless of settings. It
installs three more hooks and registers two listeners:

| Target | RVA | Detour | Installer | Name string |
| --- | --- | --- | --- | --- |
| `PlayLayer::init` | `0x3A5B00` | `0x1801846C0` | `0x1800AB5B0` | `0x1805454E8` |
| `PlayLayer::setupHasCompleted` | `0x3A66D0` | `0x1801846D0` | `0x1800AB850` | `0x1805454F8` |
| `UILayer::init` | `0x4CCB00` | `0x180184710` | `0x180167DB0` | `0x180575861` |

Tag listeners registered from the same body: **169** (`0xA9`) through `0x1800733D0` with
callback `0x1801848A0`, and **170** (`0xAA`) through `0x180074110` with callback
`0x1801848E0`. Both callbacks are identical (`0x1801848A0..0x1801848D3` and
`0x1801848E0..0x180184912`): play-layer lookup, `[playlayer+0x3540]` → `0x180185B50`, then
`jmp 0x180184BA0(node, dl=0)`.

`0x1801846D0` is the show/hide entry from gameplay: it calls `0x1801A59F0`, resolves the
play layer, gets the interface through `0x180185B50`, and tail-jumps `0x180184BA0`.

### Interface construction and visibility

`0x180184710` (`.pdata` `0x180184710..0x18018486F`) allocates `0x1E8` bytes
(`0x1803A50B4`), constructs `0x1801E85C0`, writes class pointer `[obj+0] = 0x180578CF8` and
vtable slots `+0x140/+0x148/+0x150/+0x158/+0x160/+0x198` =
`0x180579108/0x180579168/0x180579178/0x180579198/0x1805791B8/0x1805791D8`, zeroes
`+0x1C0..+0x1D0`, runs `0x180185370`, and either autoreleases or calls `[vtable+8](1)`.

`0x180184BA0(this, layer)` (`.pdata` `0x180184BA0..`) shows the interface with
`[vtable+0x140](1)`; if tag 169 is true it stops the actions on `+0x1C0`, hides `+0x1C0`
and `+0x1C8`, shows `+0x1D0`; otherwise it fades `+0x1D0` out with
`CCFadeTo::create(0.25, 64)` after `CCDelayTime::create(1.0)`.

### Changelog rows mapped by this evidence

| Changelog row | Tag(s) read |
| --- | --- |
| Added StartPos Switcher interface | none (unconditional install `0x1800B2DEE` → `0x180184490`) |
| Added Hide Interface to StartPos Switcher | 169 (static map entry 4, store `0x180064B32`) |
| Added Switch After Death to StartPos Switcher | 168 (static map entry 17, store `0x180065055`) |
| Added StartPos Switcher keybind options | 131 / 132 |
| Added StartPos Switcher quick switch (both wordings) | 149 + table `0x1805793A8` (tags 150-165) |

UNVERIFIED: the direct binding of tag 166 to `ALLOW_RATED` (only elimination from the four
remaining dropdown keys); the meaning of tag 170 (registered next to 169 with an identical
callback — the only other key in the family is `INTERFACE_SIZE`, but no read of 170 was
found in the interface code); which site populates dictionary groups 1-5; the semantics of
`[layer+0x878]+0x388/+0x38C/+0x390`, `[layer+0x3188]`, `[layer+0x3230]`, `[layer+0x3880]`,
`[layer+0x3888]`, `[layer+0x3540]`; the identity of `0x18015A2F0` and `0x180158F50`; and
whether the 17-entry shortcut table's duplicated `0x96` at index 10 is intentional or a
typo in the shipped table.
