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
