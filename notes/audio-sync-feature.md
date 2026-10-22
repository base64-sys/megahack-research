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
