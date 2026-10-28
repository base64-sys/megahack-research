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
## Evidence - Compact Lists is tag 473 and shrinks the level cells (2026-10-28)

The row names no settings key in the inventory, but the image has exactly one mechanism that
changes the layout of a level list cell, and it belongs to tag 473 (`0x1D9`, no static label;
registered at `0x1800B2F1E` in `FUN_1800AFAE0`, callback `FUN_1801863B0`).

| Target | RVA | Detour | Installer |
| --- | --- | --- | --- |
| `BoomListView::init` | `0x3C020` | `FUN_1801864E0` | `FUN_180186750` |
| `LevelCell::loadLocalLevelCell` | `0xB0D70` | `FUN_180186560` | `FUN_1801866B0` |
| `LevelCell::loadCustomLevelCell` | `0xAE230` | `FUN_180186620` | `FUN_180186B60` |

**The flag.** `FUN_1801864E0` sets `DAT_18080C8A8` to 1 only when the list's type argument is
5 or 6, or equals `0x10` (`if (1 < x - 5 && x != 0x10) flag = 0`), calls the original
`FUN_1801E55A0`, then clears the flag. So the compaction applies to specific list types, not
every list.

**What the cell detours change** (all keyed on that flag, all through node ids):

- `FUN_180186560` (local cells): child `main-layer` -> child `level-name` -> virtual `+0xA0`
  with `0x3F19999A` = **0.6f**, i.e. the name label is scaled down.
- `FUN_180186620` (custom cells): child `main-layer`, then `level-place` -> virtual `+0x140`
  with `0` (hidden), `percentage-label` -> virtual `+0xE0` with `0x41C80000` = **25.0f**, and
  `completed-icon` -> `+0xE0` with the same 25.0f, i.e. the number is dropped and the remaining
  nodes are moved to x = 25.

**Two angles.** Behaviour: this is the only code in the image that rescales or hides parts of a
level cell, and the three node ids it touches (`level-name`, `level-place`,
`percentage-label`, `completed-icon`) are each referenced only from these detours. Naming:
`UNIVERSAL/HACK_COMPACT_LISTS` is the only compact-list key in the settings tree, and the row
"Added Compact Lists" is the only changelog wording about list layout.

**Conclusion.** Tag 473 is Compact Lists. Tag 465 hooks the same area but changes no cell
content; it is the list-background toggle in the next section.

UNVERIFIED: which virtual slots `+0xA0`, `+0xE0`, `+0x140` and `+0x28` are (read as setScale,
setPositionX, setVisible and setAnchorPoint/setPosition respectively), what list type ids 5, 6
and `0x10` mean in GD 2.2081, and whether the 25.0f offset is a width derived from the hidden
`level-place` node.

## Evidence - Transparent Lists is tag 465 and hides the list background (2026-10-28)

The second list hook group in `_hook_sites.tsv` is registrar `0x18018C040`, callback
`FUN_18018C040` (`0x18018C040`, 208 bytes, zero callers), which installs two hooks at order
`0xFFFFFFE0` and removes them again on the disable path:

| Target | Host RVA | Detour | Installer |
| --- | --- | --- | --- |
| `GJListLayer::init` | `0x27F8D0` | `FUN_18018C110` | `FUN_18018C1A0` |
| `CustomListView::loadCell` | `0xAB610` | `FUN_18018C170` | `FUN_18018C2F0` |

Both detours end in the same call, `(**(code **)(*(obj + 0x198) + 0x28))(obj + 0x198, 0)` -
one virtual slot on the node the list layer keeps at `+0x198`, with a zero argument.
`FUN_18018C110` runs the original `init` first and only touches the node when `init`
returned true; `FUN_18018C170` resolves the owning list through `[cell+0x218]` and applies
the same call to its `+0x198` node after calling the original loader. So the list's own
background node is neutralised once at creation and again for every cell that loads.

**Naming.** Three angles point at the v9.0.0 row "Added Transparent Lists":

1. Tag adjacency inside one feature family: 463 (`0x1CF`, Custom Background master),
   464 (`0x1D0`, its refresh listener) and 465 (`0x1D1`) are consecutive runtime tags
   registered from the same region, and the row "Added Custom Background" is documented as
   the successor to Transparent BG (`cosmetic-feature.md`, tag map above).
2. Capability: this is the only code in the image that touches a list's own background
   node, and the only changelog wording about list transparency is that row.
3. Bounded negative: tag 473's cell detours (Compact Lists, mapped 2026-10-28) are the only
   other list hooks, and they change label scale and position rather than a node at `+0x198`.

The older combined row v6.1.0 "Added Transparent BG & List mods (by WEGFan)" names the same
two mechanisms - the background half is tag 463 and the list half is tag 465 - so it is a
predecessor wording of two already-mapped toggles.

UNVERIFIED: which virtual slot `+0x28` is (read as a visibility or opacity setter on the
`+0x198` node, host class layout absent), what the node at `[list+0x198]` is in GD 2.2081,
and which settings key writes tag 465 (`_settings.tsv` has no accessor for it, so the value
reaches the callback through the registration path only).

