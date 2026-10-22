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
- UNVERIFIED: whether `NoclipTint` is drawn through the render hook cluster or
  a separate path.
- UNVERIFIED: the exact meaning of the accuracy and death limits.
- UNVERIFIED: the JSON key→tag binding for tag 73 (candidate:
  `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE`) and for the Safe Mode flags 481/482.
