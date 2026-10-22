# Iconic feature cluster

- **Question** — which code and definitions implement Mega Hack's Iconic
  (garage icon effects) features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — tab construction

- `FUN_18011FFE0` (`0x18011FFE0`) builds the `ICONIC` tab. It repeatedly
  calls `hackpro::define` for prefixes such as `ICONIC/DISABLE`,
  `ICONIC/VANILLA_PROGRESS_BAR`, `ICONIC/GENERAL`, `ICONIC/EFFECTS`,
  `ICONIC/WAVE`, `ICONIC/RAND`, then opens sections and adds numeric rows.
  See [definition-keys.md](definition-keys.md).
- `ICONIC/VANILLA_PROGRESS_BAR` is one of the keys used in that tab.

## Evidence — special icon effects

- `FUN_1801FC6C0` (`0x1801FC6C0`) builds icon effect UI and references:
  - `ICONIC/SPECIAL/PREVIEW`
  - `ICONIC/SPECIAL/INVERTED`
  - `ICONIC/SPECIAL/SPEED/PRE`
  - `ICONIC/SPECIAL/SPEED/SUF`
- `FUN_1801261C0` (`0x1801261C0`) references `ICONIC/PREVIEW/ICONS`.

## Evidence — shortcut registration

- `FUN_18003C470` and `FUN_1800B6770` register the `HOME/SHORTCUT/ICONIC`
  action, so Iconic can be reached from the shortcut system as well as from its
  own tab.

## Evidence — assets

- Iconic uses shipped sprites such as `iconic.png`, `iconic_btn.png`,
  `iconic_ghost_btn.png` and `iconic_particles_btn.png` (listed in `mod.json`).
  Those are Absolllute's assets and are not reproduced here.

## Open questions

- UNVERIFIED: which settings keys gate each Iconic effect. No `ICONIC/...`
  setting keys appear in the observed `hackpro.json` sample, so they may be
  definition-only or loaded from a different profile.
- UNVERIFIED: the exact render path for each effect (particles, ghost trail,
  glow, wave trail). `FUN_1801FC6C0` builds the UI; the gameplay/render hooks
  are not yet tied to individual toggles.
- UNVERIFIED: whether `ICONIC/DISABLE` disables the whole feature or only the
  preview.
