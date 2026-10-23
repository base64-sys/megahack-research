# NoClip feature cluster

- **Question** — which code and settings implement Mega Hack's NoClip features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

| Setting key | Observed value | Role |
| --- | --- | --- |
| `LEVEL/NOCLIP/DROPDOWN/PLAYER_1` | True | Enable NoClip for player 1 |
| `LEVEL/NOCLIP/DROPDOWN/PLAYER_2` | True | Enable NoClip for player 2 |
| `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE` | True | Noclip safe-mode/disable-progress option (tag binding UNVERIFIED) |
| `LEVEL/NOCLIP/DROPDOWN/TINT/CHECKBOX` | True | Tint overlay |
| `LEVEL/NOCLIP/DROPDOWN/TINT/COLOUR` | 2303 | Tint colour |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/ACCURACY_LIMIT_ON` | False | Accuracy limit |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/DEATH_LIMIT_ON` | True | Death limit |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/ACCURACY_LIMIT` | 99.0 | Accuracy limit value |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/DEATHS_LIMIT` | 11 | Death limit value |
| `LEVEL/NO_COLLISION/DROPDOWN/HAZARDS` | True | Disable hazard collision |
| `LEVEL/NO_COLLISION/DROPDOWN/SLOPES` | True | Disable slope collision |
| `LEVEL/NO_COLLISION/DROPDOWN/SOLIDS` | True | Disable solid collision |
| `LEVEL/NO_COLLISION/DROPDOWN/SPECIALS` | True | Disable special collision |
| `STATUS/NOCLIP_ACCURACY/DROPDOWN/PREFIX` | False | Status label prefix |
| `STATUS/NOCLIP_DEATHS/DROPDOWN/UNITS` | True | Status label units |

## Evidence — implementation

- `core::NoclipTracker` is constructed by `FUN_180201300`
  (`0x180201300`).
- `NoclipTint` is constructed by `FUN_18017A500`
  (`0x18017A500`), which installs multiple vtables.
- `HACK_NOCLIP_LIMITS` and `CHECKBOX_NOCLIP_*` strings are built in
  `FUN_180062940` (`0x180062940`), the UI builder.
- `Noclip: {:.2f}%` is formatted in `FUN_180152010`
  (`0x180152010`).

## Evidence — Disable Progress gate

- `CHECKBOX_NOCLIP_DISABLE_PROGRESS` = tag 73 (`0x49`), Map A. Its entry is inserted at
  store VA `0x180064D8D` in the 21-entry static map; see
  [static-hack-label-map.md](static-hack-label-map.md).
- `FUN_18019B5F0` (`0x18019B5F0`) reads it through `FUN_18002DE50` and returns true when
  the tag is set and `FUN_180199520() < 1.0` — when some level time was attributed to a
  cheat frame (`DAT_18080CB68 == 1`).
- The effect is progress-side: `FUN_180199DF0` (the `GJGameLevel::savePercentage` hook)
  returns without calling the original, and `FUN_180199E30` (the `PlayLayer::levelComplete`
  hook) runs the original with `PlayLayer+0x3230` set. See
  [safe-mode-feature.md](safe-mode-feature.md).
- UNVERIFIED: which JSON key writes tag 73. `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE` is the only
  observed Noclip safety key and is `True` in the sampled profile, but no recovered code
  binds that path to 73.

## Open questions

- UNVERIFIED: which functions read each NoClip setting tag and apply the
  collision bypass.

## Evidence — Noclip Limits (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18017A290` (`0x18017A290`) also implements the Noclip Limits feature
(when `HACK_NOCLIP_LIMITS` = tag 83 is enabled):

1. **Accuracy limit** (tag 0x55 = 85, `CHECKBOX_NOCLIP_ACCURACY_LIMIT`):
   if enabled, reads tag 0x54 (84, a double) as the accuracy threshold. Computes
   `FUN_180199560()` (current noclip accuracy) and sets `bVar12 = (accuracy <= threshold)`.

2. **Death limit** (tag 0x57 = 87, `CHECKBOX_NOCLIP_DEATH_LIMIT`):
   if enabled, reads tag 0x56 (86, a uint) as the death count threshold. If it
   matches `FUN_1801994E0()` (current death count), sets
   `bVar12 |= (FUN_1801994D0() ^ 1)` (cheat flag from previous frame).

3. If `bVar12 != 1` (limits not exceeded): skips the tint overlay (goto
   `LAB_18017a37d`).

### Tag map (limits)

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x53 | 83 | `isHackEnabled` (Map B) | `HACK_NOCLIP_LIMITS` | Master toggle |
| 0x55 | 85 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_ACCURACY_LIMIT` | Accuracy limit toggle |
| 0x54 | 84 | `FUN_180003D80` (double accessor) | (not in static list) | Accuracy threshold value |
| 0x57 | 87 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_DEATH_LIMIT` | Death limit toggle |
| 0x56 | 86 | `FUN_18002E4C0` (uint/int accessor) | (not in static list) | Death count threshold

UNVERIFIED: which JSON keys bind to tags 0x54 and 0x56; the exact semantics of
`FUN_180199560` (current accuracy) and `FUN_1801994E0` (death count).
- UNVERIFIED: whether `NoclipTint` is drawn through the render hook cluster or
  a separate path.
- UNVERIFIED: the exact meaning of the accuracy and death limits.
- UNVERIFIED: the JSON key→tag binding for tag 73 (candidate:
  `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE`) and for the Safe Mode flags 481/482.

## Evidence — Noclip Tint (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18017A220` (`0x18017A220`) registers `FUN_18017A290` (`0x18017A290`)
as a hook on `PlayLayer::destroyPlayer` (RVA `0x3B39D0`).

`FUN_18017A290` handles the tint overlay:
- If `isHackEnabled(HACK_NOCLIP_LIMITS = 83, 0x53)` is false: skips the
  limits check.
- Reads `CHECKBOX_NOCLIP_PLAYER_1` (tag 77) and `CHECKBOX_NOCLIP_PLAYER_2`
  (tag 78) to determine which player triggered the collision.
- If `CHECKBOX_NOCLIP_TINT` (tag 75) is true: applies a tint overlay using
  `DAT_18080C710` (a CCNode) with fade/hide animations. The tint colour
  comes from tag 0x4C (76, a spinner value).
- When players differ and player 2 is enabled (`PlayLayer+0x422 == 1`), the
  tint is applied to the other player's node.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x53 | 83 | `isHackEnabled` (Map B) | `HACK_NOCLIP_LIMITS` | NoClip limits master toggle |
| 0x4D | 77 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_PLAYER_1` | Player 1 tint toggle |
| 0x4E | 78 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_PLAYER_2` | Player 2 tint toggle |
| 0x4B | 75 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_TINT` | Tint overlay toggle |
| 0x4C | 76 | `FUN_18002E4C0` (uint/int accessor) | (not in static list — runtime-assigned) | Tint colour value

UNVERIFIED: which JSON keys bind to tag 0x4C; the exact tint colour encoding;
the semantics of `DAT_18080C710` and the animation parameters.

## Evidence — Noclip Accuracy display (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180151FC0` (`0x180151FC0`) registers `FUN_180152010` (`0x180152010`)
as a hook on `EndLevelLayer::showLayer` (RVA `0x134CA0`).

`FUN_180152010` draws the "Noclip: {:.2f}%" label:
- If `isHackEnabled(tag 0x1F1 = 497)` is true: returns immediately (hides the
  accuracy display).
- If false AND there's a current level: draws the accuracy label with
  checkpoint/position data.

The function also reads `HACK_NOCLIP` (tag 72) and uses `FUN_18008A510` for
settings access. It formats "Noclip: {:.2f}%" using `FUN_1803A50B4` for
string allocation.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1F1 | 497 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | "Hide Noclip Accuracy" toggle (inverted) |
| 0x48 | 72 | `isHackEnabled` (Map B) | `HACK_NOCLIP` | Master NoClip toggle (read for context)

UNVERIFIED: which JSON key binds to tag 0x1F1; whether this is the "Noclip
Accuracy" feature or a separate "hide accuracy" toggle; the exact rendering path
for the label.
