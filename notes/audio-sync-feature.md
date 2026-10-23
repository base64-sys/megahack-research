# Audio sync / deafen feature cluster

- **Question** — which settings and code implement Mega Hack's Auto Music Sync and
  Auto Deafen features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

Auto Music Sync:

- `LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/NO_LEGACY_SYNC` (bool, True)
- `LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/THRESHOLD` (int, 40)

Auto Deafen:

- `LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_IN_PRACTICE` (bool, True)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_WITH_STARTPOS` (bool, True)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/UNDEAFEN_ON_PAUSE` (bool, True)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_AT` (decimal, 35.0)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/UNDEAFEN_AT` (decimal, 100.0)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/SHORTCUT` (bytes, [45])

## Evidence — UI registration

- `FUN_180062940` (`0x180062940`) registers:
  - `HACK_AUTO_MUSIC_SYNC`
  - `HACK_AUTO_DEAFEN`
  - `SHORTCUT_AUTO_DEAFEN`
  - `SPINNER_AUTO_MUSIC_SYNC`

## Changelog features

- "Fixed Auto Music Sync not accounting for Music Offset"
- "Added Auto-Update" is unrelated but appears in the same changelog cluster.

## Open questions

- UNVERIFIED: which functions read each Auto Music Sync / Auto Deafen tag.
- UNVERIFIED: how the threshold and deafen percentages are applied to GD's
  audio engine.
- UNVERIFIED: whether Auto Deafen uses a hook on `FMODAudioEngine::update` or
  only settings-driven behaviour.
- UNVERIFIED: the meaning of `SHORTCUT_AUTO_DEAFEN`'s byte value `[45]`.

## Evidence — Corrective Music Sync (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18019C420` (`0x18019C420`) registers three things:

1. `FUN_18019C6D0` as a change listener for tag 0x1F7 (503).
2. `FUN_18019C7E0` as a hook on `FMOD::ChannelControl::setPitch`.
3. `FUN_18019C800` as a hook on `FMODAudioEngine::setMusicTimeMS` (RVA 0x5C1C0).

`FUN_18019C6D0` (`0x18019C6D0`) is the corrective scaler: when `*param_2 == 1`
and `isHackEnabled(tag 0x1F6 = 502)` is true, calls `FUN_18019C400()` (returns
`DAT_1807F9420`, the corrective pitch) and scales all FMOD channels' pitch by
`corrective / previous`. Stores the new value in `DAT_1807F9428`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1F6 | 502 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Enable corrective pitch scaling |
| 0x1F7 | 503 | listener-registered (tag 0x1F7) | (not in static list) | Trigger for corrective recalculation |
| — | — | global `DAT_1807F9420` | — | Corrective pitch value |

UNVERIFIED: which JSON keys bind to tags 0x1F6 and 0x1F7; the semantics of
`FUN_18019C7E0` and `FUN_18019C800`; the exact corrective pitch algorithm.

## Evidence — Pitch Shifter (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18018A740` (`0x18018A740`) registers `FUN_18018A800` as a change
listener for tag 0x1CC (460). When tag 460 changes:

`FUN_18018A800` checks `isHackEnabled(HACK_PITCH_SHIFTER = 459, 0x1CB)`. If
enabled, calls `FUN_18020DA00(value)` which applies the pitch to audio channels
0–4 via `FUN_18020DAD0`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1CB | 459 | `isHackEnabled` (Map B) | `HACK_PITCH_SHIFTER` | Master toggle |
| 0x1CC | 460 | (listener-registered, value is double) | (not in static list — runtime-assigned) | Pitch value (applied to FMOD channels 0–4)

UNVERIFIED: which JSON key binds to tag 0x1CC; the exact pitch range; whether
the listener registration uses `hackpro::Config`'s change-listener mechanism.
