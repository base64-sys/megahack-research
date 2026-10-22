# Show Hitboxes feature cluster

- **Question** — which settings and code implement Mega Hack's Show Hitboxes
  features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

- Master/option toggles:
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/AXIS_ALIGNED_BOX`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/FILL`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/HAZARDS/CHECKBOX`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/HIDE_USED`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/PASSABLE/CHECKBOX`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/PLAYER`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/PLAYER_ROTATION`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/SECTION`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/SELECTED_OBJECTS`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/SOLIDS/CHECKBOX`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/SPECIALS/CHECKBOX`
- Colours:
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/HAZARDS/COLOUR`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/PASSABLE/COLOUR`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/SOLIDS/COLOUR`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/SPECIALS/COLOUR`
- Numeric tuning:
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/OPACITY`
  - `LEVEL/SHOW_HITBOXES/DROPDOWN/STROKE_SIZE`
- Show Hitboxes on death:
  - `LEVEL/SHOW_HITBOXES_ON_DEATH/DROPDOWN/DEATH_OBJ`
  - `LEVEL/SHOW_HITBOXES_ON_DEATH/DROPDOWN/NOCLIP`
- Show Hitbox Trail:
  - `LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/HIGHLIGHT_HALFTICKS`
  - `LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/REQUIRE_SHOW_HITBOXES`
  - `LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/UNCAPPED_EDITOR_LENGTH`
  - `LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/OPACITY`
  - `LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/MAX_DRAW`

## Evidence — UI registration

- `FUN_180062940` (`0x180062940`) is a static initializer that builds two
  `{label, integer}` maps: 85 `HACK_*` names and 21 `CHECKBOX_*`/`NO_LEGACY_SYNC`
  `OPTIONS_BOOL` names. The three Show Hitboxes labels in the first map are:
  - `HACK_SHOW_HITBOXES` = 192 (`0x0C0`)
  - `HACK_SHOW_HITBOXES_ON_DEATH` = 216 (`0x0D8`)
  - `HACK_SHOW_HITBOXES_TRAIL` = 224 (`0x0E0`)

  The same initializer also builds a 21-entry `CHECKBOX_*`/`NO_LEGACY_SYNC` `OPTIONS_BOOL`
  map; its 19 accessor-listed ids are confirmed Map A config tags. The 85-entry table is the
  Map B hack-flag map, and the three values above are read by the Map B getter
  `FUN_18008F420` (`isHackEnabled`). See
  [static-hack-label-map.md](static-hack-label-map.md).
- Changelog features include:
  - "Selected Objects Only to Show Hitboxes"
  - "Show Hitboxes Hide Activated Objects option"
  - "Show Hitboxes Player Rotation option"
  - "Show Hitboxes Player Collison Blocks"
  - "Show Hitbox Trail Opacity"
  - "CBS Tick Highlighting in Show Hitbox Trail"
  - "Requires Show Hitboxes option to Show Hitbox Trail"

## Open questions

- UNVERIFIED: which functions read each Show Hitboxes setting tag and draw
  the hitboxes.
- UNVERIFIED: whether the trail and on-death variants share the same
  rendering helper.
- UNVERIFIED: the exact colour encoding (observed values are integers).
- UNVERIFIED: whether `HIDE_USED` affects rendering or only the UI.
