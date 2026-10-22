# Status labels feature cluster

- **Question** — which settings and hooks implement Mega Hack's status labels?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

- `STATUS/HIDE` — master hide toggle.
- `STATUS/COSMETIC/OPACITY`, `STATUS/COSMETIC/SCALE` — cosmetic scaling.
- Field positions: `STATUS/FIELDS/0..23/DROPDOWN/POSITION`.
- Field format: `STATUS/FIELDS/8/FORMAT` (observed `'{best_run}'`).
- Units: `STATUS/ATTEMPTS/DROPDOWN/UNITS`,
  `STATUS/CPS/DROPDOWN/UNITS`, `STATUS/FPS/DROPDOWN/UNITS`,
  `STATUS/FRAME/DROPDOWN/UNITS`, `STATUS/JUMPS/DROPDOWN/UNITS`,
  `STATUS/NOCLIP_DEATHS/DROPDOWN/UNITS`.
- Toggles: `STATUS/CPS/DROPDOWN/MAX`, `STATUS/CPS/DROPDOWN/TOTAL`,
  `STATUS/NOCLIP_ACCURACY/DROPDOWN/PREFIX`.
- Best Run status fields are covered in [best-run-feature.md](best-run-feature.md).

## Evidence — hooks

Status labels are drawn in the level lifecycle, not through a dedicated render hook
in the recovered cluster. The relevant hooks are:

- `PlayLayer::postUpdate` (`0x1801992B0`) accumulates progress/time globals.
- `PlayLayer::updateProgressbar` (`0x1800A83B0`, `0x180165040`) is hooked twice
  and converges on `FUN_1800AB850`.
- `PlayLayer::toggleProgressbar` (`0x180190E50`) converges on the same helper.

## Pattern

Status labels are configuration-driven: positions and units are stored as typed
config values, while the lifecycle hooks only provide timing/progress. This
matches the nxUI model where rows are layout handles and text comes from the
definition manager.

## Open questions

- UNVERIFIED: which functions read each `STATUS/FIELDS/*` key and format the
  label text.
- UNVERIFIED: the complete set of status field keys beyond the observed 24.
- UNVERIFIED: whether status labels are updated every frame or only on
  lifecycle events.
- UNVERIFIED: the exact format-string grammar used by `STATUS/FIELDS/8/FORMAT`.
