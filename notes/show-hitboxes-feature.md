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

## Evidence — implementation (recovered 2026-10-23 via REA/Ghidra 12.1.2)

The draw entry point is `FUN_180203F60` (`0x180203F60`). Three render paths call it
(`0x180158740`, `0x180158830`, `0x1801588F0`), each looking up the draw node by ID
`0x1E6813A` on the layer at `PlayLayer+0x3770`. Each caller clears/refreshes first,
then calls `FUN_180203F60(playLayer, drawNode)`.

### Gate — `FUN_180203EE0` (`0x180203EE0`)

Returns 1 (draw) when either:

1. `isHackEnabled(HACK_SHOW_HITBOXES = 192, 0xC0)` is true (master toggle), OR
2. `isHackEnabled(HACK_SHOW_HITBOXES_ON_DEATH = 216, 0xD8)` is true AND
   `PlayLayer+0x309C == 0` AND
   (`PlayLayer+0x324A != 0` OR
    (`getConfigVar<bool>(tag 0xD9 = 217)` is true AND `FUN_1801994D0()` != 0)).

`FUN_1801994D0` returns `DAT_18080CB69` (the previous frame's cheat-attributed flag),
so branch 2 fires on the frame after a cheat-attributed death, or when `+0x324A` is set.

### Draw — `FUN_180203F60` (`0x180203F60`)

1. `CCDrawNode::clear(drawNode)`.
2. If the gate above is false, return.
3. Call `FUN_180204000(playLayer, drawNode)` then `FUN_180204150(playLayer, drawNode)`.

### `FUN_180204000` (`0x180204000`) — main hitbox pass

- If on-death mode (`HACK_SHOW_HITBOXES_ON_DEATH` on, `+0x309C == 0`,
  `+0x324A != 0` OR (tag 0xD9 on AND `FUN_1801994D0()` != 0)) AND
  `getConfigVar<bool>(tag 0xDA = 218)` is true AND `FUN_1801995B0()` != 0
  (death object pointer exists): draw that single death object via `FUN_180203070`.
  - `FUN_1801995B0` returns `DAT_18080CB80`, the death-causing object pointer
    recorded by the `destroyPlayer` callback (`0x1801993F0`).
- Else if `getConfigVar<bool>(tag 0xC1 = 193)` is true: iterate the selected-object
  vectors at `PlayLayer+0x31A0`/`+0x31A8` and `+0x31C0`/`+0x31C8`, calling
  `FUN_180203070` for each.
- Else (fill/axis-aligned off): call `FUN_180204230` with the four values at
  `PlayLayer+0x308C..+0x3098` (colour/opacity/stroke parameters).

### `FUN_180204150` (`0x180204150`) — player hitbox pass

If `getConfigVar<bool>(CHECKBOX_HITBOXES_PLAYER = 206, 0xCE)` is true:
- Draw `PlayLayer+0xDA0` (player 1) via `FUN_180202BD0`.
- If `PlayLayer+0xDA8 != 0` AND `PlayLayer+0x422 == 1` (player 2 enabled):
  draw `PlayLayer+0xDA8` as well.

### Tag map for this feature

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0xC0 | 192 | `isHackEnabled` (Map B) | `HACK_SHOW_HITBOXES` | Master toggle |
| 0xD8 | 216 | `isHackEnabled` (Map B) | `HACK_SHOW_HITBOXES_ON_DEATH` | On-death variant |
| 0xC1 | 193 | `getConfigVar<bool>` (Map A) | (not in 21-entry static list — runtime-assigned) | **Corrected 2026-10-26:** gates the selected-object iteration; the old "fill/axis-aligned box" label is withdrawn |
| 0xCE | 206 | `getConfigVar<bool>` (Map A) | `CHECKBOX_HITBOXES_PLAYER` | Player hitbox toggle |
| 0xD9 | 217 | `getConfigVar<bool>` (Map A) | (not in static list — runtime-assigned) | "Only Death Object" / cheat-frame gate |
| 0xDA | 218 | `getConfigVar<bool>` (Map A) | (not in static list — runtime-assigned) | "Death Object Only" |
| 0xD4 | 212 | `getConfigVar<double>` (Map A) | (not in static list — runtime-assigned) | Hitbox Multiplier: cached as float `0x1808195AC`, read only inside the box-geometry function `0x180202BD0` |

Tags 0xC1, 0xD9, 0xDA are read through the Map A accessor but are absent from the
21-entry `CHECKBOX_*` static table in `FUN_180062940`; they are therefore runtime-assigned
Map A tags (likely from `hackpro.json`), not compiled-in defaults. UNVERIFIED: which
JSON keys / UI rows bind to these three tags.

### PlayLayer fields used

| Offset | Type | Role |
| --- | --- | --- |
| `+0x308C..+0x3098` | 4 × int | Axis-aligned box colour/opacity/stroke parameters (passed to `FUN_180204230`) |
| `+0x309C` | char | Editor/selected-objects mode flag (gates on-death path) |
| `+0x31A0`/`+0x31A8` | pointer pair | Selected-objects vector (begin/end) |
| `+0x31C0`/`+0x31C8` | pointer pair | Second selected-objects vector |
| `+0x324A` | char | On-death state flag |
| `+0x3770` | pointer | Layer node (CCNode), used to look up draw node `0x1E6813A` |
| `+0xDA0` | pointer | Player 1 node |
| `+0xDA8` | pointer | Player 2 node |
| `+0x422` | char | Player 2 enabled flag |

### Open questions

- UNVERIFIED: which JSON keys bind to tags 0xC1 (193), 0xD9 (217), 0xDA (218).
- UNVERIFIED: the exact colour encoding (observed values are integers).
- UNVERIFIED: whether `HIDE_USED` (settings key `LEVEL/SHOW_HITBOXES/DROPDOWN/HIDE_USED`)
  affects rendering or only the UI.
- UNVERIFIED: whether the trail variant (`HACK_SHOW_HITBOXES_TRAIL = 224`) shares
  `FUN_180203070` or has its own draw path.
- UNVERIFIED: the semantics of `PlayLayer+0x309C` and `+0x324A`.

## Evidence — Hitbox Trail draw node (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_1801589F0` (`0x1801589F0`) creates three draw nodes on the layer at
`PlayLayer+0x3770`:

1. "hitbox" (ID `0x1E6813A`) — always created, added at z-order 0x5DC.
2. "hitbox-trail" (ID `0x1E6813B`) — always created, added at z-order 0x5DD.
3. "hitbox-trail" (ID `0x1E6813C`) — created with blend func 0x302/0x303, its
   visibility set via vtable +0x140 based on `isHackEnabled(tag 0x168 = 360)`.

Tag 0x168 is a runtime-assigned Map B tag (not in the 85-entry static list).
It controls whether the third draw node (trail overlay) is visible.

The main trail drawing is in `FUN_1801809A0` (tag 224 = `HACK_SHOW_HITBOXES_TRAIL`),
which draws into node 0x1E6813A. The node 0x1E6813C (controlled by tag 360) is a
separate trail overlay.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x168 | 360 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Hitbox trail overlay visibility toggle

UNVERIFIED: which JSON key binds to tag 0x168; the difference between node
0x1E6813B and 0x1E6813C; the exact rendering path for the trail overlay.

## Evidence — Show Hitbox Trail implementation (recovered 2026-10-23)

The trail draw entry point is `FUN_1801809A0` (`0x1801809A0`), called from the
same three render paths as the main hitbox draw (node ID `0x1E6813A`).

### Gate

1. `isHackEnabled(HACK_SHOW_HITBOXES_TRAIL = 224, 0xE0)` must be true; otherwise
   the function returns immediately.
2. If `getConfigVar<bool>(tag 0xE3 = 227)` is true ("Requires Show Hitboxes"):
   `FUN_180203EE0(playLayer)` must return non-zero (i.e. Show Hitboxes gate open).
   Tag 0xE3 is a runtime-assigned Map A tag (not in the 21-entry static list).

### Draw

- Reads opacity via `FUN_180003D80(settings, tag 0xE4 = 228)` (a double accessor),
  divides by 100.0.
- Iterates a global trail point buffer (`DAT_18080C7D8` base pointer,
  `DAT_18080C7F0` count, `DAT_18080C7F8` remaining) and draws each point via
  `FUN_180202BD0(drawNode, &point, opacity, colourByte)`.
- Each point carries position (x, y), colour bytes, and a boolean flag that
  switches between two point layouts (with/without extended fields).

### Tag map (trail)

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0xE0 | 224 | `isHackEnabled` (Map B) | `HACK_SHOW_HITBOXES_TRAIL` | Master toggle |
| 0xE3 | 227 | `getConfigVar<bool>` (Map A) | (not in static list — runtime-assigned) | "Requires Show Hitboxes" gate |
| 0xE4 | 228 | `FUN_180003D80` (double accessor) | (not in static list) | Trail opacity (0–100) |

UNVERIFIED: which JSON keys bind to tags 0xE3 and 0xE4; the exact type of
`FUN_180003D80` (likely `getConfigVar<double>` or a spinner accessor); the
structure of the global trail buffer and its producer.

## Evidence — Selected Objects Only (2026-10-26)

The `Added Selected Objects Only to Show Hitboxes` row (v9.1.0) is the runtime-assigned
tag 193. It is read inside the main hitbox pass, not registered by its own callback: the
whole hitbox renderer is installed unconditionally from the shared initialiser.

| Field | Value |
| --- | --- |
| Install site | `0x1800AFB2B` in `FUN_1800AFAE0`, `call 0x1801582C0` (no `lea` xref anywhere) |
| Hook | `PlayLayer::updateVisibility` RVA `0x3AF3A0`, detour `0x180158740`, installer `0x1800AB9F0`, name `0x180574D90` |
| Read site | `FUN_180204000` (main hitbox pass): `0x180204093 mov edx,0xC1` + `call 0x18002DE50` |
| Settings key | `SHOW_HITBOXES/SELECTED_OBJECTS` in the settings tree |

The path in `FUN_180204000`:

```
0x180204079  mov edx,0xD9      ; tag 217 (death-object-only gate)
0x180204081  call 0x18002DE50
0x180204086  test al,al
0x180204088  je 0x180204093
0x18020408A  call 0x1801994D0  ; predicate; on true skip to 0x180204042
0x180204093  mov edx,0xC1      ; tag 193 = selected objects only
0x180204098  mov rcx,rbx
0x18020409B  call 0x18002DE50
0x1802040A0  test al,al
0x1802040A2  je 0x180204109    ; false -> normal all-object iteration
0x1802040A4  mov rbx,[rdi+0x31A0]   ; selected-object array begin
0x1802040AB  mov r14,[rdi+0x31A8]   ; selected-object array end
0x1802040B2  cmp rbx,r14
0x1802040B5  je 0x1802040D7
0x1802040C0  mov r8,[rbx]           ; one entry
0x1802040C3  mov rcx,rdi
               ...                 ; -> 0x180203070 (per-object hitbox draw)
```

So with tag 193 set the pass iterates the selection array at `[PlayLayer+0x31A0 ..
+0x31A8)` and feeds each entry to `0x180203070`; with it clear the code falls through to
`0x180204109`, the ordinary all-object path. That is exactly "selected objects only", and it
is the only read of tag 193 in the image.

### Correction (2026-10-26)

The tag-map row above previously called `0xC1` a "fill/axis-aligned box toggle". That is
**withdrawn**: the only site that reads tag 193 is `0x18020409B`, and it selects the object
list to iterate, not a fill mode. The fill/axis-aligned behaviour is among the option tags
cached by the config refresh (`0x1802024E0` region), but which tag is which there is not
proven, so no replacement name is asserted.

### Corroborating reads in the same renderer

The config-cache refresh (`0x1802024E0` region) caches eleven booleans and one float used by
the draw functions: tags 195/198/201/204 (four category checkboxes), 207, 208, 209, 210,
211, 229, and double 212 converted to a float at `0x1808195AC`. Four colour doubles
196/199/202/205 sit beside them. That accounts for all eleven boolean option keys in the
settings tree, but the tag-to-name binding for the six non-category booleans remains
UNVERIFIED.

UNVERIFIED: the element type of the `[PlayLayer+0x31A0]` array and whether `+0x31A8` is an
end pointer or a capacity bound; the JSON key that binds tag 193 (only the settings-tree name
is known); and the names of tags 207/208/209/210/211/229.

## Evidence — Hitbox Multiplier (2026-10-26)

Read from `absolllute.hackmega_original.dll` (SHA-256
`1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`). The option is a
**double** tag: 212 (`0xD4`). It is the only non-boolean tag in the hitbox option family and
the only value the config refresh caches as a float.

### Configuration path

| Step | Address | What happens |
| --- | --- | --- |
| Read | `0x1802025B4` | `getConfigVar<double>(212)` through `0x180003D80` |
| Convert | after the read | `cvtsd2ss` to a 32-bit float |
| Cache | store | float written to `0x1808195AC` |

The neighbouring stores in the same refresh function are the boolean options: tags
195/198/201/204 to `0x1808195A0..0x8195A3`, 207 to `0x1808195A4`, 208 to `0x1808195A5`,
210 to `0x1808195A6`, 209 to `0x1808195A7`, 211 to `0x1808195A8`, 229 to `0x1808195B0`.
Tag 212 is the odd one out in both type and cache slot width, which is what identifies it.

### Consumption: the box-geometry function

`0x180202BD0..0x18020306E` (`.pdata`, 0x49E bytes) is the routine that turns one object's
rect into hitbox points. A full-`.text` sweep for RIP-relative operands to the cache block
produced 42 sites; **every read of `0x1808195AC` falls inside this function**, and nowhere
else in the image.

```
0x180202D95  movss xmm2, [0x1808195AC]     ; branch A
0x180202DA7  mulss xmm0, [0x180534410]     ; 0.5f half-extent
0x180202DB5  movss xmm2, [0x1808195AC]     ; same branch, second use
             ... call 0x1805331B0 with r8d=4   ; four-point helper
0x180202E0B  movss xmm0, [0x1808195AC]     ; branch B
0x180202E27  mulss xmm0, [0x180534410]     ; 0.5f
0x180202E67  movss xmm0, [0x1808195AC]
0x180202E6?  shufps xmm3, xmm0, 0          ; broadcast the multiplier
0x180202EAA  addps  xmm4, xmm2             ; xmm2 = [rdi+0x10]
             ... call 0x180202610 with the multiplier as stack float [rsp+0x20]
```

The rect fields used are `[rdi]`, `[rdi+8]`, `[rdi+0x10]` and `[rdi+0x18]`; the half-extent
is scaled by the 0.5f constant `0x180534410`. Adjacent gates in the same function read the
boolean caches: `0x180202D64 cmp byte [0x1808195A4],1` (tag 207) together with
`cmp byte [rdi+0x40],1`, and `0x180202D89 cmp byte [0x1808195A8],0` (tag 211) plus
`0x180202E04 movzx eax, byte [0x1808195A8]`.

### Why this is "Hitbox Multiplier"

The changelog row is the only hitbox bullet about sizing, and the settings tree carries a
dedicated family `LEVEL/HITBOX_MULTIPLIER/DROPDOWN/{HAZARDS,SAWS,SOLIDS}/ENABLED` (all
`True`) in [settings-contents.md](settings-contents.md) lines 37–39. Among the twelve hitbox
option tags (eleven booleans plus 212), only 212 is a double, only 212 is cached as a float,
and only 212 feeds the geometry function as a scale/broadcast factor. The three category
toggles are therefore *not* among the booleans already identified as UI checkboxes, but their
own tags are not located.

UNVERIFIED: the exact arithmetic — the cached float is loaded, broadcast, and added to rect
coordinates (`addps`), not multiplied against the rect, so tag 212 behaves like an outward
expansion in the quoted instructions even though the changelog and key call it a multiplier;
the three `HITBOX_MULTIPLIER/DROPDOWN/*/ENABLED` tag bindings; the meanings of `[rdi]`,
`[rdi+8]`, `[rdi+0x10]`, `[rdi+0x18]` and `[rdi+0x40]`; and whether the two branches
(`0x180202D95` and `0x180202E0B`) correspond to axis-aligned versus rotated boxes.
