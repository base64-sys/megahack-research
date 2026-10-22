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
