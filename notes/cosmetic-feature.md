# Cosmetic feature cluster

- **Question** — which settings and code implement Mega Hack's cosmetic features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

Boolean toggles (`COSMETIC/HACK_*`):

- `COSMETIC/HACK_BALL_ROTATION_BUG`
- `COSMETIC/HACK_CLASSIC_PARTICLES`
- `COSMETIC/HACK_CLASSIC_PULSE`
- `COSMETIC/HACK_CLASSIC_WAVE_TRAIL`
- `COSMETIC/HACK_HIDE_PAUSE_BUTTON`
- `COSMETIC/HACK_NO_END_SHAKE`
- `COSMETIC/HACK_NO_NEW_BEST_POPUP`
- `COSMETIC/HACK_NO_PULSE`
- `COSMETIC/HACK_NO_RESPAWN_FLASH`
- `COSMETIC/HACK_NO_ROBOT_FIRE`
- `COSMETIC/HACK_NO_SPIDER_DASH`
- `COSMETIC/HACK_NO_TRAIL`
- `COSMETIC/HACK_NO_WAVE_PULSE`
- `COSMETIC/HACK_SHOW_TOTAL_ATTEMPTS`
- `COSMETIC/HACK_SOLID_WAVE_TRAIL`
- `COSMETIC/HACK_STARTPOS_RESET_CAMERA`
- `COSMETIC/HACK_STOP_TRIGGERS_ON_DEATH`
- `COSMETIC/HACK_TRAIL_ALWAYS_ON`
- `COSMETIC/HACK_WAVE_TRAIL_ON_DEATH`
- `COSMETIC/NO_PARTICLES/HACK_NO_PARTICLES`
- `COSMETIC/TRAIL_CUTTING/HACK_TRAIL_CUTTING`
- `COSMETIC/WAVE_PULSE_SIZE/HACK_WAVE_PULSE_SIZE`

Icon randomiser:

- `COSMETIC/ICON_RANDOMISER/DROPDOWN/COLOURS`
- `COSMETIC/ICON_RANDOMISER/DROPDOWN/ICONS`

Accurate percentage:

- `COSMETIC/ACCURATE_PERCENTAGE/DROPDOWN/BETTER_CALCULATION`
- `COSMETIC/ACCURATE_PERCENTAGE/DROPDOWN/DECIMALS`

Other cosmetic settings:

- `COSMETIC/NO_PARTICLES/DROPDOWN` (bytes)
- `COSMETIC/TRAIL_CUTTING/DROPDOWN/RATE` (decimal)
- `COSMETIC/WAVE_PULSE_SIZE/DROPDOWN/SIZE` (decimal)

## Evidence — UI registration

- `FUN_180062940` (`0x180062940`) registers many of these as labels,
  including `HACK_NO_NEW_BEST_POPUP`, `HACK_NO_TRAIL`,
  `HACK_TRAIL_ALWAYS_ON`, `HACK_NO_WAVE_PULSE`,
  `HACK_SOLID_WAVE_TRAIL`, `HACK_NO_ROBOT_FIRE`,
  `HACK_NO_SPIDER_DASH`, `HACK_NO_END_SHAKE`,
  `HACK_NO_RESPAWN_FLASH`, `HACK_NO_PULSE`,
  `HACK_CLASSIC_PARTICLES`, `HACK_CLASSIC_PULSE`,
  `HACK_CLASSIC_WAVE_TRAIL`, `HACK_BALL_ROTATION_BUG`,
  `HACK_HIDE_PAUSE_BUTTON`, `HACK_STARTPOS_RESET_CAMERA`,
  `HACK_STOP_TRIGGERS_ON_DEATH`, `HACK_SHOW_TOTAL_ATTEMPTS`,
  `HACK_NO_PARTICLES`, `HACK_TRAIL_CUTTING`,
  `HACK_WAVE_PULSE_SIZE`.

## Open questions

- UNVERIFIED: which functions read each cosmetic setting tag and apply the
  visual change.
- UNVERIFIED: whether the classic-particle / classic-pulse toggles are
  render-hook or gameplay-hook features.
- UNVERIFIED: whether `NO_PARTICLES`'s byte array is a per-particle-type mask.
- UNVERIFIED: whether `TRAIL_CUTTING/RATE` and `WAVE_PULSE_SIZE/SIZE` are
  consumed by the render hooks or by GD-side settings.
- UNVERIFIED: whether `HACK_SHOW_TOTAL_ATTEMPTS` is cosmetic or status-related.

## Evidence — Custom Background (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180186D10` (`0x180186D10`) registers two things:

1. `FUN_180186FC0` as a change listener for tag 0x1D0 (464).
2. `FUN_180187000` as a hook on `cocos2d::CCTransitionScene::initWithDuration`.

`FUN_180186FC0` (`0x180186FC0`): if `isHackEnabled(tag 0x1CF = 463)` is true,
calls `FUN_180187030(directorScene, 1)` which hides the background node (finds
"background" child of the first scene object and modifies it).

`FUN_180187030` (`0x180187030`): modifies a background CCNode — if param_2 == 1,
hides it (via vtable +0x140 with 0). If param_2 == 0, shows it.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1CF | 463 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Master toggle (Custom Background) |
| 0x1D0 | 464 | listener-registered (tag 0x1D0) | (not in static list) | Trigger for background refresh

UNVERIFIED: which JSON keys bind to tags 0x1CF and 0x1D0; the exact background
modification (hide/show vs. colour change); the vtable's owning class.

## Evidence — Menu Snow in December (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180154AD0` (`0x180154AD0`) checks if the current month is December
(`_time64` → `FUN_1801DD8C0` → month == 0xB). If so:

1. Registers `FUN_180154C10` (snow particle creator, uses `CCParticleSnow` +
   `snow.png` texture) as a hook on `MenuLayer::init` (RVA `0x333A90`).
2. Registers `FUN_1801550F0` as a change listener for tag 0x1F2 (498).

`FUN_1801550F0` toggles the snow particle system's visibility via a vtable
method at +0x140, passing `isHackEnabled(tag 0x1F2) ^ 1`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1F2 | 498 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | "Menu Snow" toggle (inverted: enabled = hidden)

UNVERIFIED: which JSON key binds to tag 0x1F2; whether the toggle is "enable snow"
or "disable snow" (the inverted logic suggests the latter); the vtable's owning class.

## Evidence — Solid Wave Trail (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_1801EECA0` (`0x1801EECA0`) controls the blend function of a wave-trail draw
node based on `HACK_SOLID_WAVE_TRAIL` (tag 396, Map B):

- If enabled: blend func = `0x30300000001` (solid/opaque), `param_1[0x1B8] = 1`.
- If disabled: blend func = `0x100000302` (blended), `param_1[0x1B8] = 0`.
- Also draws 4 points via `FUN_18014C760`.

Called from `FUN_1801EE440` (0x1801EE440), a wave-trail render function with a
switch on trail type (0–9). `FUN_1801EE440` is called from `FUN_1801224A0` and
`FUN_180123A70`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x18C | 396 | `isHackEnabled` (Map B) | `HACK_SOLID_WAVE_TRAIL` | Solid (opaque) vs blended wave trail

UNVERIFIED: the exact render path (which GD object triggers `FUN_1801EE440`);
whether the toggle affects only the blend mode or also the point count.
