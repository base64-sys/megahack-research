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

## Evidence — Show Hitbox Trail Opacity (2026-10-26)

The changelog row is implemented by one `double` tag, read once, inside the hitbox-trail draw
function. No static `HACK_*` label exists for it, so the binding rests on the accessor type,
the divide-by-100 and the argument it becomes.

| Field | Value |
| --- | --- |
| Tag | 228 (`0xE4`), runtime-assigned (absent from the 85-label static map) |
| Read | `0x180180A2D` in `0x1801809A0`, `mov edx,0xE4` → `call 0x180003D80` (`getConfigVar<double>`) |
| Consumed | `divsd xmm0, [0x180534330]` (100.0) at `0x180180A46`, then passed as `xmm2` into `0x180202BD0` per object |
| Function | `0x1801809A0` — `CCDrawNode::clear` then the trail rebuild (no `.pdata` record) |
| Only read | the full-`.text` tag scan finds exactly one site reading tag 228 |

Path in order:

```
0x1801809e4  call 0x18008a510                 ; settings context
0x1801809ec  mov edx, 0xe3                    ; 227 requires-show-hitboxes
0x1801809f4  call 0x18002de50                 ; bool
0x1801809fb  mov edx, 0xe0                    ; 224 master
0x180180a03  call 0x18008f420                 ; isHackEnabled
0x180180a0a  je 0x180180b5e                   ; trail off -> return
0x180180a15  call 0x180203ee0                 ; when 227: hitbox gate must also be true
0x180180a25  mov edx, 0xe4                    ; 228 opacity
0x180180a2d  call 0x180003d80                 ; double
0x180180a42  movapd xmm6, xmm0
0x180180a46  divsd xmm6, [0x180534330]        ; / 100.0
0x180180a7a  call 0x180202bd0                 ; (node, point, xmm2 = opacity/100)
```

The loop walks the trail buffer (`[0x18080C7F8]` count, `[0x18080C7D0]` ring, `[0x18080C7D8]`
buffer) and draws one entry per iteration with the tag-derived value as the third argument, so
the setting is a percentage scaled to 0..1 before it reaches the geometry helper.

UNVERIFIED: that `0x180202BD0` multiplies the third argument into the vertex alpha rather than
using it as a width or distance (the argument position and the `/100` are measured, the
destination arithmetic is not); and the units documented for the settings slider, which would
confirm the 0..100 range.

## Evidence - death-hitbox options and the completed hitbox tag map (2026-10-26)

The 2026-10-25 note left "eleven boolean keys, eleven boolean tags, six unbound" as an open
problem. Both halves of that sentence are now wrong, and the death options are measured but
still unnamed.

### Correction (2026-10-26): seven hitbox tags are labelled after all

The second table of `notes/static-hack-label-map.md` (the Map A `CHECKBOX_*` bank) already
names seven of them:

| Setting key | Tag | Label-map entry |
| --- | --- | --- |
| `SHOW_HITBOXES/DROPDOWN/FILL` | 211 (`0xD3`) | 2 `CHECKBOX_FILL_HITBOX`, store `0x180064A88` |
| `SHOW_HITBOXES/DROPDOWN/HAZARDS/CHECKBOX` | 198 (`0xC6`) | 3 `CHECKBOX_HAZARDS_HITBOX`, `0x180064ADD` |
| `SHOW_HITBOXES/DROPDOWN/PASSABLE/CHECKBOX` | 204 (`0xCC`) | 14 `CHECKBOX_PASSABLE_HITBOX`, `0x180064F2E` |
| `SHOW_HITBOXES/DROPDOWN/SOLIDS/CHECKBOX` | 195 (`0xC3`) | 15 `CHECKBOX_SOLIDS_HITBOX`, `0x180064F8E` |
| `SHOW_HITBOXES/DROPDOWN/SPECIALS/CHECKBOX` | 201 (`0xC9`) | 16 `CHECKBOX_SPECIALS_HITBOX`, `0x180064FEE` |
| `SHOW_HITBOXES/DROPDOWN/PLAYER` | 206 (`0xCE`) | 5 `CHECKBOX_HITBOXES_PLAYER`, `0x180064B8A` |
| `SHOW_HITBOXES/DROPDOWN/SELECTED_OBJECTS` | 193 (`0xC1`) | none (runtime-assigned) |

So the config refresh's cache slots are now named: `0x1808195A8` (tag 211, ten readers) is
FILL, `0x1808195A0..A3` are SOLIDS/HAZARDS/SPECIALS/PASSABLE, and the value consumed at the
head of the geometry function is FILL rather than a generic flag. Tag 206 is independently
confirmed by the draw path (below), which is why the key-to-tag row above can be stated as
fact rather than inference: the label table gives the tag, and the code shows the behaviour.

Five bool tags remain for four remaining keys (`AXIS_ALIGNED_BOX`, `HIDE_USED`,
`PLAYER_ROTATION`, `SECTION`): 207, 208, 209, 210, 229. One of the five is therefore **not** a
`SHOW_HITBOXES` key, and the "exactly eleven tags" premise is withdrawn.

### Tag 206 draws the player box

`FUN_180204150` (0x180204150..0x180204221, `sub rsp,0xB0`, `/GS` cookie `0x1807F7040`) reads
bool 206 at `0x18020417F`. When set it builds a rect with `0x180203C20` and submits it through
`0x180202BD0` at scale `1.0` (`0x180534360`):

- player 1 slot `[layer+0xDA0]`, rect buffer `[rsp+0x64]`, call `0x1802041B2`;
- player 2 slot `[layer+0xDA8]` only when `[layer+0x422] == 1`, rect buffer `[rsp+0x20]`, call
  `0x1802041EA`.

No other tag is read in this function, so the player box is a single-key feature.

### Death-hitbox path, measured

Gate `FUN_180203EE0` (0x180203EE0..0x180203F52, no `.pdata`):

```
call 0x18008A510; mov edx,0xC0; call 0x18008F420   ; show hitboxes on -> return 1
call 0x18008A510; mov edx,0xD8; call 0x18008F420   ; death hitboxes off -> return 0
cmp byte [layer+0x309C],0 ; jne -> return 0
cmp byte [layer+0x324A],0 ; jne -> return 1
mov edx,0xD9; call 0x18002DE50 ; false -> return 0
jmp 0x1801994D0                                     ; returns byte [0x18080CB69]
```

Draw `FUN_180204000` (0x180204000..0x1802040F6):

```
mov edx,0xD8; call 0x18008F420 ; off -> 0x180204093 (selected/all-object pass)
cmp byte [layer+0x309C],0 ; jne -> 0x180204093
cmp byte [layer+0x324A],0 ; je 0x180204079
0x180204042: mov edx,0xDA; call 0x18002DE50 ; off -> 0x180204093
             call 0x1801995B0   ; qword [0x18080CB80] = stored death object
             ... r8 = that object; jmp 0x180203070   ; single-object draw
0x180204079: mov edx,0xD9; call 0x18002DE50 ; off -> 0x180204093
             call 0x1801994D0 ; zero -> 0x180204093
             jmp 0x180204042
```

Read as control flow: tag **218** (`0xDA`) is a mandatory condition for the death-object-only
draw, tag **217** (`0xD9`) is the alternative precondition used when `[layer+0x324A] == 0`, and
the object drawn is whatever `0x1801995B0` returns from the mod global `0x18080CB80`. These are
the only two tags in the whole death path, and there are exactly two death settings keys
(`LEVEL/SHOW_HITBOXES_ON_DEATH/DROPDOWN/DEATH_OBJ` False, `.../NOCLIP` True).

A write-site scan of `.text` (every instruction with a memory operand at displacement `0x324A`
or `0x309C` and write access) found **no** writer in the mod image, so both bytes and the flag
at `0x18080CB69` appear to be host-owned state. That is why the names cannot be settled from
this artifact.

UNVERIFIED: which of 217/218 is `DEATH_OBJ` and which is `NOCLIP` — the two candidate readings
are "218 = death-object-only, 217 = noclip precondition" and its inverse, and the defaults
(`DEATH_OBJ` False, `NOCLIP` True) fit the second reading as well as the first; the meanings of
`[layer+0x309C]`, `[layer+0x324A]` and `byte [0x18080CB69]`; what `0x180203070` does with the
third argument; whether the death-object-only mode also requires the player to be dead at all.
The five changelog rows for these options therefore stay `cluster` with this section as the
record of why.
