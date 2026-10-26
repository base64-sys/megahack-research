# Render hooks cluster

- **Question** — which rendering paths does Mega Hack hook, and what do they share?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Filter `recovered/functions/_hook_sites.tsv` for GD methods whose names contain
   `draw`, `Render`, `Shader`, `Camera`, `Visibility`, `Ghost`, `Streak`, `CircleWave`,
   `swapBuffers`, or `drawScene`.
2. Cross-check registrar and callback VAs against `notes/hook-dispatch.md`.

## Evidence — render hook sites

| Registrar VA | GD method | Target | Callback VA | Callback kind | Helper |
| --- | --- | --- | --- | --- | --- |
| `0x18014ED70` | `cocos2d::CCEGLView::swapBuffers` | name-resolved | `0x18014F9D0` | function | `0x18014FEE0` |
| `0x180156D00` | `cocos2d::CCDirector::drawScene` | name-resolved | `0x180156ED0` | function | `0x180157D50` |
| `0x180170DC0` | `cocos2d::CCParticleSystemQuad::draw` | name-resolved | `0x180170FB0` | data | `0x180170FC0` |
| `0x180171DB0` | `cocos2d::CCMotionStreak::draw` | name-resolved | `0x180171FA0` | data | `0x180171FB0` |
| `0x180171620` | `ShaderLayer::performCalculations` | RVA `0x4880B0` | `0x1801716F0` | data | `0x180171710` |
| `0x180171620` | `ShaderLayer::visit` | RVA `0x488A60` | `0x180171700` | data | `0x180171710` |
| `0x180171170` | `GJBaseGameLayer::lightningFlash` | RVA `0x246D00` | `0x1801711E0` | data | `0x1801711F0` |
| `0x1801713E0` | `PlayLayer::updateVisibility` | RVA `0x3AF3A0` | `0x180171450` | function | `0x1800AB9F0` |
| `0x1801582C0` | `PlayLayer::updateVisibility` | RVA `0x3AF3A0` | `0x180158740` | function | `0x1800AB9F0` |
| `0x1801582C0` | `LevelEditorLayer::updateVisibility` | RVA `0x2D8E70` | `0x180158830` | function | `0x180158CC0` |
| `0x180197020` | `GJBaseGameLayer::updateCamera` | RVA `0x23BCF0` | `0x180197440` | label | `0x180172EC0` |
| `0x18019ACB0` | `CCCircleWave::draw` | RVA `0x42E40` | `0x18019AD00` | function | `0x18019AEA0` |
| `0x1800A83B0` | `PlayerObject::toggleGhostEffect` | RVA `0x39EE30` | `0x18010BD80` | function | `0x1800AB3D0` |
| `0x180169AE0` | `PlayerObject::toggleGhostEffect` | RVA `0x39EE30` | `0x180169B50` | data | `0x1800AB3D0` |
| `0x180167A60` | `PlayerObject::toggleVisibility` | RVA `0x38B440` | `0x180167B10` | label | `0x180167B20` |
| `0x180172280` | `PlayerObject::activateStreak` | RVA `0x3A0090` | `0x1801722F0` | function | `0x1800AB230` |

## Pattern

- The global render loop is hooked at two choke points:
  - `cocos2d::CCDirector::drawScene` (`0x180156D00`);
  - `cocos2d::CCEGLView::swapBuffers` (`0x18014ED70`).
- Particle and streak rendering are intercepted as data-slot callbacks rather than
  normal functions (`CCParticleSystemQuad::draw`, `CCMotionStreak::draw`), which means
  their callback bodies are not split into standalone recovered functions.
- Visibility is hooked repeatedly across ownership contexts:
  - `PlayLayer::updateVisibility` twice (`0x1801582C0`, `0x1801713E0`);
  - `LevelEditorLayer::updateVisibility` once (`0x1801582C0`);
  - `PlayerObject::toggleVisibility` once (`0x180167A60`).
- Ghost and streak effects are hooked from two different registrars each
  (`PlayerObject::toggleGhostEffect` at `0x1800A83B0` and `0x180169AE0`;
  `PlayerObject::activateStreak` at `0x180172280`), suggesting the same visual feature
  is observed from more than one call site.
- `ShaderLayer` is hooked as a pair: `performCalculations` and `visit` share helper
  `FUN_180171710`, so its calculations and its draw are treated as one unit.

## Evidence — No Particles (recovered 2026-10-24 via REA/Ghidra 12.1.2 + pe_disasm)

`HACK_NO_PARTICLES` (tag 402, `0x192`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b11a8`:

```
0x1800b11a8  mov  edx, 0x192          ; tag = HACK_NO_PARTICLES
0x1800b11ad  mov  rcx, rax            ; resolver
0x1800b11b0  mov  r8, rsi             ; context structure
0x1800b11b3  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801703b0` (`0x1801703b0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: builds the module name `"libcocos2d.dll"` inline and the mangled
  export name from `.rdata` at `0x180575CE5` —
  `?create@CCParticleSystemQuad@cocos2d@@SAPEAV12@PEBD_N@Z`
  (`cocos2d::CCParticleSystemQuad::create(char const*, bool)`) — resolves it via
  `FUN_18010a250` / `FUN_18010a5a0`, then registers the detour at `0x1801705A0` through
  `FUN_180170bc0` with the friendly hook name `cocos2d::CCParticleSystemQuad::create`
  (string at `0x180575CBF`).
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

Verified 2026-10-25 by disassembling `FUN_1801703b0` with `pe_disasm.py`: the two strings
above were read from the image with `scripts/pe_rva.py`, and the `movups` sources
(`rip+0x4058c3` etc.) were recomputed by hand — the first arithmetic attempt used the
wrong `rip`-relative base and produced plausible but wrong text, which is why the note now
names the exact `movups` addresses. The detour address is the `lea r8,[rip+0x10c]` at
`0x18017048d`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x192 | 402 | (registered via FUN_18008f910) | `HACK_NO_PARTICLES` | Master toggle (No Particles) |

UNVERIFIED: what the detour does with the created `CCParticleSystemQuad` (whether it
returns null, suppresses the draw, or alters a property); which JSON key binds to tag
0x192.

## Hook registration convention (established 2026-10-25)

The toggles in this cluster share one shape, and reading it correctly is what separates a
verified hook from a guess:

1. In `FUN_1800afae0`, `mov edx, <tag>` / `mov rcx, <resolver>` / `mov r8, <context>` /
   `call 0x18008f910` registers the tag → callback binding.
2. The callback reads `*param_2`; `1` installs, anything else unregisters via
   `FUN_180108a80`.
3. The target is resolved one of two ways:
   - **Same-image RVA:** `mov edx, <rva>` then `call 0x18010a370` (e.g.
     `mov edx, 0x3a0090` for `PlayerObject::activateStreak`).
   - **Export by mangled name:** module string (built inline with `movabs`) plus a
     `.rdata` mangled symbol, resolved by `FUN_18010a5a0`.
4. The detour is the `lea r8,[rip+…]` operand of the hook-creation call; the friendly name
   is the `lea r9,[rip+…]` operand. The name is for logging, **not** the resolution key —
   e.g. `cocos2d::CCParticleSystemQuad::create` names a hook that was resolved by
   `?create@CCParticleSystemQuad@cocos2d@@SAPEAV12@PEBD_N@Z`.

A note that only gives the friendly name and no RVA or mangled symbol has not yet shown
which function is hooked.

### Verification tool

`scripts/tag_callback_map.py` reproduces the tag → callback table from the instruction bytes,
with no third-party packages:

```
python3 scripts/tag_callback_map.py samples/absolllute.hackmega/absolllute.hackmega_original.dll
```

It finds every `mov edx,<tag>` / `mov rcx,rax` / `mov r8,rsi` / `call 0x18008f910` sequence in
`.text` and resolves the callback as the nearest preceding `lea rcx,[rip+disp]`, which
is the rule step 1 above establishes. Running it on 2026-10-25 yields **125 registrations**
(125 distinct tags); all twenty tag→callback pairs recorded in this note and in the
per-feature notes match its output, apart from the No Shake row corrected in this batch.
That is the check that catches the adjacency error: the callback belongs to the block
*before* the tag, not after it.

**Correction (2026-10-25).** The tool originally scanned only
`0x1800AFAE0`–`0x1800B2000` and reported 79 registrations. That window is not the whole
initialiser: the function continues to `0x1800B3550`, and 46 further registrations sit
past it (plus two outside the initialiser entirely, at `0x1800A8206` and `0x18018ACDC`).
The window-bounded count undercounted by 46 and made the map look complete; the tool now
scans the whole section, and `notes/hook-registrations.md` carries all 125 rows.

The remaining **leads are the unlabelled tags**: 46 of the 125 have no `HACK_*` label, and
almost all are still unidentified — they are the cheapest next step for the rest of the
feature inventory.
The first pass over them is now recorded in
[hook-registrations.md](hook-registrations.md): all 125 tags with their static `HACK_*`
label (79 present, 46 runtime-assigned), callback, hook targets, detours, names and
installers — **186 same-image hooks across 111 callbacks**, 9 symbol-only callbacks, 3
registrations that resolve a callback but install no hook, and 2 whose callback could not
be located. A companion tool reads those operands from the instruction bytes:

```
python3 scripts/callback_hooks.py samples/absolllute.hackmega/absolllute.hackmega_original.dll 0x180169330
```

It anchors on the `call 0x18010A370` that resolves an RVA, so it reports the resolution key
rather than a plausible-looking immediate, and bounds each callback at the next one in the
registration table. As a check it reproduces the four hand-derived rows (No Shake, No Death
Effect, No Wave Trail, No Shaders) exactly. One new fact came out of the first run: No Death
Effect installs a **second** hook, on `PlayLayer::destroyPlayer` (RVA `0x3B39D0`).


## Evidence - Hide Player (verified 2026-10-25)

`HACK_HIDE_PLAYER` (tag 386, `0x182`) registers at `0x1800b0aaf` with callback
`FUN_180167A60`, which hooks `PlayerObject::toggleVisibility` (RVA `0x38B440`, name string
`"PlayerObject::toggleVisibility"`, installer `FUN_180167B20`). The detour is only two
instructions:

```
0x180167b10  31 d2        xor    edx, edx      ; visible = false
0x180167b12  e9 ...       jmp    0x1801a9b70   ; PlayerObject::setVisible(this, false)
```

The detour discards the incoming `edx` (the visibility the caller asked for) and always
forwards `false`, so the player object cannot become visible while the toggle is on. The
callback also applies the toggle directly: after installing, it reads both player slots
`PlayLayer+0xDA0` and `+0xDA8`, computes `!*param_2` (`xor bl,1` at `0x180167AC9`) and
calls the same `0x1801A9B70` on each player object, which is why a dual is handled in one
pass. The two-argument reading of `0x1801A9B70` as a visibility setter is consistent with
both call sites; UNVERIFIED: its symbol, which is not resolved by name anywhere in this
callback.

This is the first row promoted from the [hook-registrations.md](hook-registrations.md)
table on detour evidence, and it is `mapped` in
[feature-inventory.md](feature-inventory.md).

### One detour left for the game-hooks note

The same pass read four detours. Hide Pause Button and Hide Pause Menu were settled below,
Stop Triggers on Death is a gameplay hook and now lives in
[gameplay-hooks.md](gameplay-hooks.md#evidence--stop-triggers-on-death-2026-10-25), and
Wave Trail Fix is settled in the next section.

## Evidence — Wave Trail Fix (2026-10-25)

Tag 411 (`0x19B`, `HACK_WAVE_TRAIL_FIX`, static-map entry 83, store VA `0x180064855`) is
registered at `0x1800B094A`. Callback `FUN_180166A80` installs one hook:

| Target | RVA | Detour | Installer | Order |
| --- | ---: | --- | --- | --- |
| `HardStreak::updateStroke` | `0x2B8EF0` | `0x180166AF0` | `0x180167020` | `0xBD8` |

The detour is a **complete replacement**: `.pdata` bounds it to `0x180166AF0..0x180166C33`
(0x143 bytes) and the body never calls a gateway for `0x2B8EF0`, so the vanilla stroke code
does not run on this path. It clears the draw node and rebuilds the geometry itself. The
cocos2d calls were resolved from their thunks with
[`scripts/import_names.py`](../scripts/import_names.py) (method in
[host-abi.md](host-abi.md#resolving-the-ff-25-import-thunks-2026-10-25)); no name below is
guessed from an address.

```
0x180166B0D  cmp  byte [rsi+0x1BA], 1        ; only this stroke type is redrawn
0x180166B14  jne  0x180166C02                ; otherwise leave the frame alone
0x180166B1D  call 0x1805330B0                ; CCDrawNode::clear()
0x180166B22  mov  rcx, [rsi+0x1A0]           ; the stroke's CCArray
0x180166B29  call 0x180532410                ; CCArray::count()
0x180166B30  je   0x180166C02                ; nothing to draw
0x180166B36  lea  rdi, [rsi+0x140]           ; embedded node sub-object
0x180166B47  call [rax+0x20]                 ; slot +0x20 = a byte, 0 -> return
0x180166B52  movss xmm6, [rsi+0x1B0]
0x180166B5A  mulss xmm6, [rsi+0x1B4]         ; base width
0x180166B6F  mov  ebx, eax                   ; that byte again
0x180166B7B  call [rax+8]                    ; slot +8 -> pointer to a 3-byte colour
0x180166B8C  call 0x1801F17E0                ; colour + alpha -> 16-byte ccColor4F
0x180166B91  movss xmm1, [0x18054C698]       ; 6.0f  (bytes 00 00 c0 40)
0x180166B99  mulss xmm1, xmm6                ; 6 x base width
0x180166BA3  call 0x180166C40                ; draw ribbon 1 in the node's colour
0x180166BA8  cmp  byte [rsi+0x1B8], 0
0x180166BAF  jne  0x180166C02                ; second ribbon disabled
0x180166BB1  call [rax+0x20]                 ; the byte again (opacity)
0x180166BC1  cvtsi2ss xmm0, eax
0x180166BC5  mulss xmm0, [0x180572B04]       ; 0.65f (bytes 66 66 26 3f)
0x180166BCD  cvttss2si r8d, xmm0             ; alpha = (int)(0.65 x opacity)
0x180166BD2  mov  word [rsp+0x25], 0xFFFF    ; white
0x180166BD9  mov  byte [rsp+0x27], 0xFF
0x180166BEB  call 0x1801F17E0                ; white + alpha -> ccColor4F
0x180166BF0  addss xmm6, xmm6                ; 2 x base width
0x180166BFD  call 0x180166C40                ; draw ribbon 2, white at 65% opacity
```

The drawing helper `0x180166C40` (`.pdata` `0x180166C40..0x180167015`, 956 bytes, exactly one
caller — this detour) takes `(HardStreak* this, float width, ccColor4F const* fill)`. It walks
the point array with `CCArray::objectAtIndex`, pairs consecutive points, computes each
segment's normal, builds a four-vertex quad whose half-width is `width * 0.5` (`0x180534410`
is `0x3F000000`), and issues `cocos2d::CCDrawNode::drawPolygon(verts, 4, fill, 0, transparent,
0)`.

Measured consequence: after clearing the node the detour emits **two** ribbons over the same
polyline — one at `6 x base width` in the node's colour, and, when `[this+0x1B8] == 0`, one at
`2 x base width` in white at `(int)(0.65 x opacity)` alpha. Because the helper halves the
width, the visible half-widths are `3 x base` and `1 x base`.

UNVERIFIED: what the gating bytes `+0x1BA` (must be 1) and `+0x1B8` (must be 0 for the white
ribbon) mean; whether `+0x1B0 * +0x1B4` is the vanilla stroke width or a mod-set scale; the
identity of the virtual slots `+0x8` and `+0x20` (their return shape matches `getColor()` and
`getOpacity()`, and the second return is consumed as the alpha, but they are not named from
this binary); and what the vanilla `updateStroke` did, since the host image is not part of the
sample set. The wide-plus-core reading of the visual result is an inference from the two width
constants, not an observation of the rendered trail.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x19B | 411 | `isHackEnabled` (Map B) | `HACK_WAVE_TRAIL_FIX` | Replace the wave trail's stroke redraw |


## Evidence — Hide Pause Button and Hide Pause Menu (2026-10-25)

Both toggles are `HACK_*` labels in the static map, and both detours end in the **same
virtual call**: slot `+0x140` with `edx = 0`. That repetition is the evidence — a shared
visibility idiom, not two unrelated behaviours.

| Tag | Static label | Registration | Callback | Target | Detour |
| ---: | --- | --- | --- | --- | --- |
| `0x181` | `HACK_HIDE_PAUSE_BUTTON` | `0x1800B0B26` | `FUN_180167D10` | `UILayer::init` RVA `0x4CCB00` | `0x180167D80` |
| `0x19A` | `HACK_HIDE_PAUSE_MENU` | `0x1800B0B9D` | `FUN_180167FA0` | `PauseLayer::customSetup` RVA `0x37C870` | `0x1801680A0` |

Hide Pause Button, in full:

```
0x180167D89  call 0x1801E9E10            ; returns bool in al
0x180167D8E  test al, al
0x180167D90  je   0x180167DA9            ; not our layer -> leave it alone
0x180167D92  mov  rcx, [rsi+0x238]       ; the child object
0x180167D99  mov  r8,  [rcx]             ; its vtable
0x180167D9C  xor  edx, edx               ; argument = false
0x180167DA0  call [r8+0x140]             ; hide it
```

Hide Pause Menu calls the original setup `0x1801A2C30` first and then hides **the layer
itself** the same way:

```
0x1801680A8  call 0x1801A2C30
0x1801680AD  mov  rax, [rsi]
0x1801680B0  mov  rax, [rax+0x140]
0x1801680BA  xor  edx, edx
0x1801680C1  jmp  rax
```

So "Hide Pause Button" hides the pause button child at `UILayer+0x238`, and "Hide Pause
Menu" hides the whole `PauseLayer` after it has been populated. **UNVERIFIED:** the identity
of vtable slot `+0x140` (the shape is consistent with `setVisible(false)`, but the slot is
not named from this binary) and whether an invisible `PauseLayer` still receives touches,
which decides whether "Hide Pause Menu" is a screenshot aid or a functional change.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x181 | 385 | `isHackEnabled` (Map B) | `HACK_HIDE_PAUSE_BUTTON` | Hide the pause button in `UILayer` |
| 0x19A | 410 | `isHackEnabled` (Map B) | `HACK_HIDE_PAUSE_MENU` | Hide the pause layer after setup |

The general lesson stands: a friendly name such as `UILayer::init` does not tell you what the
detour does to the layer — only the call at the end of the detour does.

## Evidence - three runtime-assigned cosmetic tags (2026-10-25)

Tags `0x1A1`, `0x1A4`, `0x1A5`, `0x1B1`, `0x1B2` and `0x1B4` are **not** in the 85-entry static
map, so they have no `HACK_*` label to lean on. Their detours were read instead, and in
most cases the detour is the whole mechanism:

| Tag | Target | Detour | Detour bytes | Effect |
| ---: | --- | --- | --- | --- |
| `0x1A1` | `GJBaseGameLayer::toggleFlipped` (RVA `0x2467D0`) | `0x180169C70` | `c3` | the flip is never applied |
| `0x1A4` | `RingObject::spawnCircle` (RVA `0x4A1DB0`) | `0x18016A3E0` | `c3` | the ring/orb circle is never spawned |
| `0x1A5` | `PlayerObject::spawnPortalCircle` (RVA `0x397760`) | `0x1801713C0` | `c3` | the portal circle is never spawned |
| `0x1B1` | **`Added Frozen Animations`** - mapped (2026-10-26) | Both `CCAnimatedSprite` animation entry points are replaced by a body that only destroys the by-value name argument, so no sprite animation can start. See [Frozen Animations and No Dash Fire](#evidence---frozen-animations-and-no-dash-fire-2026-10-26). |
| `0x1B2` | `PlayerObject::playSpiderDashEffect` (RVA `0x395170`) | `0x180171A30` | `c3` | the spider-dash effect is never played |

A one-byte `ret` is the same suppression idiom as No Death Effect
(`0x180169400`), so for these four the mechanism is established, not inferred.

`0x1B4` is different: its detour (`0x1801692D0`) keeps the player and hides one child
node — it loads `[this+0x660]`, tests it for null, then calls vtable slot `+0x140` with
`edx=0` (the same slot the pause detours use), and clears `[this+0x660]`'s own `+0x140`
entry afterwards. So the node is hidden rather than the effect being suppressed.

### Which changelog feature is which

The changelog rows are `Added No Orb Ring`, `Added No Portal Circle` (v9.0.0),
`Added No Circle Effect` (v9.0.7), `Added No Spider Dash`, `Added No Dash Fire` and
`Added No Flip Portal hack`. Binding a tag to a row needs more than a plausible target,
because two of those rows compete for the same target:

| Tag | Binding | Basis |
| ---: | --- | --- |
| `0x1B2` | **`Added No Spider Dash`** — mapped | Friendly name `PlayerObject::playSpiderDashEffect`, and the settings key `COSMETIC/HACK_NO_SPIDER_DASH` is present in [settings-contents.md](settings-contents.md). Two independent angles. |
| `0x1A4` | **`Added No Orb Ring`** — mapped | The hook's friendly name is `RingObject::spawnCircle`, i.e. the ring spawned by a ring/orb object, and the competing circle-family row is portal-specific (its fix note names mini and wave portals, not orbs). The binding assumes the friendly name reflects behaviour, which the code does not independently prove. |
| `0x1A5` | **`Added No Portal Circle`** - mapped (corrected 2026-10-26) | Both changelog rows describe a portal circle and only one tag hooks `spawnPortalCircle`, so this was left unpromoted at first. The tag-allocation order then separated them: 421 sits inside the v9.0.0 group (420 No Orb Ring, 429/430 Accurate Percentage) while a v9.0.7 feature would have to sit near 437, whose batch detour covers `spawnPortalCircle`, `spawnDualCircle` and `CCCircleWave::create` together. The inference and its limits are written out in [No Portal Circle and No Ghost Trail](#evidence---no-portal-circle-and-no-ghost-trail-2026-10-26). |
| `0x1A1` | UNVERIFIED: `Added No Flip Portal hack` | The mechanism matches (flip suppressed), but no settings key or label corroborates the binding, and the row could equally be handled by the editor-side flip path. Not promoted. |
| `0x1B4` | **`Added No Dash Fire`** - mapped (corrected 2026-10-26) | The hook target name settles what the node hide belongs to: the detour is installed on `PlayerObject::updateDashAnimation` (name string `0x1805454C6`), so the hidden child is the dash animation's effect node. `+0x660` being the fire sprite specifically is still inferred from the wording, and that gap is written out in [Frozen Animations and No Dash Fire](#evidence---frozen-animations-and-no-dash-fire-2026-10-26). |

`0x1AD` is a separate, larger case and is not a cosmetic no-op: it hooks both
`PlayLayer::updateProgressbar` (`0x3B23C0`, detour `0x180165110`, a real function with a
`0xd0` frame) and `PlayLayer::getCurrentPercent` (`0x3B3950`, detour `0x1801655E0`), which
is the shape of the percentage feature rather than a "No …" toggle.

## Evidence - No Shake (corrected 2026-10-25)

`HACK_NO_SHAKE` (tag 409, `0x199`) is registered as a hook in `FUN_1800afae0`
at `0x1800b1560`:

```
0x1800b1560  mov  edx, 0x199          ; tag = HACK_NO_SHAKE
0x1800b1565  mov  rcx, rax            ; resolver
0x1800b1568  mov  r8, rsi             ; context structure
0x1800b156b  call 0x18008f910         ; hash the tag, bind the callback
```

Its callback is **`FUN_1801718C0`**, *not* `FUN_180171620`. The callable is materialised
immediately before the registration:

```
0x1800b152f  lea  rcx, [rip + 0xc038a]   ; -> 0x1801718C0, the callback
0x1800b1536  mov  [rbp + 0xdb0], rcx
0x1800b153d  lea  rdx, [rbp + 0xda8]
0x1800b1544  mov  [rbp + 0xde0], rdx     ; pointer to the {capture, callback} pair
```

A full-`.text` scan for `lea reg,[rip+disp]` resolving to `0x180171620` finds exactly one
site, `0x1800b14b8`, which belongs to the `0x185` / No Shaders block. The No Shake block's
own `lea` resolves to `0x1801718C0`, so the two toggles do **not** share a callback. The
2026-10-24 note (and the first version of this section) said they did; that was an
assumption from adjacent registration sites, contradicted by the instruction bytes.

`FUN_1801718C0` installs a hook whose target is GD RVA `0x3A5B00`, friendly name
`PlayLayer::init` (string at `0x1805454E8`), detour `0x180171930`, installer
`FUN_1800ab5b0`, order constant `0xffffffe0`:

```
0x1801718db  mov  edx, 0x3a5b00       ; GD RVA
0x1801718e3  call 0x18010a370         ; resolve target
0x1801718f8  lea  r8, [rip + 0x31]    ; -> 0x180171930, detour
0x1801718ff  lea  r9, [rip + 0x3d3be2]; -> 0x1805454E8, "PlayLayer::init"
0x18017190c  call 0x1800ab5b0         ; install
```

The detour is a *scoped setting wrapper*, not a no-op or a veto:

1. `call 0x180147ba0` obtains the context object (`FUN_180147ba0`).
2. `lea r12,[rip+0x404486]` at `0x180171952` resolves to `0x180575DDF`, the string
   `"0172"`. `FUN_180148610(ctx,"0172")` reads the current bool;
   `FUN_180149670(ctx,"0172",1)` sets it.
3. `call 0x1801a4f30` forwards the original four register arguments (`rcx, rdx, r8d, r9d`).
4. `FUN_180149670(ctx,"0172",saved)` restores the previous value; the detour returns the
   forwarded call's result.

`"0172"` has exactly one reference in `.text` — this detour — so it is a name resolved at
runtime (the `tagFor` boundary), not a statically comparable literal.

Corroboration from a second angle: the Ghidra hook-site pass
(`recovered/functions/_hook_sites.tsv`, gitignored) lists
`0x1801718C0  PlayLayer::init  0x3a5b00  rva  FUN_180171930  function  FUN_1800ab5b0`,
and separately `0x180171620  ShaderLayer::performCalculations  0x4880b0` /
`ShaderLayer::visit  0x488a60`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x199 | 409 | (registered via FUN_18008f910) | `HACK_NO_SHAKE` | Master toggle (No Shake) |

Open questions:

- UNVERIFIED: what the setting `"0172"` controls, and why it is forced only for the
  duration of `PlayLayer::init`. Hypothesis: it is the underlying no-shake flag and the
  level-start shake is applied during `PlayLayer::init`, so scoping the flag to that call
  removes the start-of-level shake and nothing else. Confirming it needs a consumer of
  `"0172"` observed at runtime.
- UNVERIFIED: which JSON key binds to tag 0x199 (the UI toggle) — distinct from `"0172"`.
- UNVERIFIED: whether "No End Shake" (v9.0.8) is a separate feature or a refinement of
  "No Shake" (v8.0.0-alpha8).

## Evidence - No Shaders (2026-10-25)

`HACK_NO_SHADERS` is **not** one of the 389 `Added …` changelog rows; it is named under
`New For 2.2 Mods`. It is still a real toggle with a real hook, so it is recorded here.

`HACK_NO_SHADERS` (tag 389, `0x185`) is registered in `FUN_1800afae0` at `0x1800b14e9`:

```
0x1800b14b8  lea  rcx, [rip + 0xc0161]   ; -> 0x180171620, the callback
0x1800b14e9  mov  edx, 0x185             ; tag = HACK_NO_SHADERS
0x1800b14f4  call 0x18008f910
```

`FUN_180171620` installs two hooks through the helper `FUN_180171710`, with order constant
`0xfffff428`:

| Target | GD RVA | Friendly name string |
| --- | --- | --- |
| `ShaderLayer::performCalculations` | `0x4880B0` | `0x180575DAB` |
| `ShaderLayer::visit` | `0x488A60` | `0x180575DCC` |

The `bl != 1` path unregisters via `FUN_180108a80`.

This was previously mis-attributed to No Shake. See the No Shake section above for the
disassembly that separates them.

UNVERIFIED: what the two `ShaderLayer` detours do (the hook-site pass records them as
`data` labels, `DAT_1801716f0` / `DAT_180171700`, so the detour bodies are still unread).

## Evidence - No Portal Lightning (recovered 2026-10-24 via REA/Ghidra 12.1.2 + pe_disasm)

`HACK_NO_PORTAL_LIGHTNING` (tag 416, `0x1A0`) is registered as a hook in `FUN_1800afae0`
at `0x1800b1296`:

```
0x1800b1296  mov  edx, 0x1A0          ; tag = HACK_NO_PORTAL_LIGHTNING
0x1800b129b  mov  rcx, rax            ; resolver
0x1800b129e  mov  r8, rsi             ; context structure
0x1800b12a1  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180171170` (`0x180171170`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `GJBaseGameLayer::lightningFlash`
  (RVA `0x246D00`) via `FUN_1801711F0`, using the string
  `"GJBaseGameLayer::lightningFlash"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1A0 | 416 | (registered via FUN_18008f910) | `HACK_NO_PORTAL_LIGHTNING` | Master toggle (No Portal Lightning) |

UNVERIFIED: which JSON key binds to tag 0x1A0; the exact behavior of the hook
(veto vs observe).

## Evidence - No Pulse (recovered 2026-10-24 via REA/Ghidra 12.1.2 + pe_disasm)

`HACK_NO_PULSE` (tag 413, `0x19D`) is registered as a hook in `FUN_1800afae0`
at `0x1800b1384`:

```
0x1800b1384  mov  edx, 0x19D          ; tag = HACK_NO_PULSE
0x1800b1389  mov  rcx, rax            ; resolver
0x1800b138c  mov  r8, rsi             ; context structure
0x1800b138f  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801713e0` (`0x1801713e0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayLayer::updateVisibility`
  (RVA `0x3AF3A0`) via `FUN_1800AB9F0`, using the string
  `"PlayLayer::updateVisibility"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x19D | 413 | (registered via FUN_18008f910) | `HACK_NO_PULSE` | Master toggle (No Pulse) |

UNVERIFIED: which JSON key binds to tag 0x19D; the exact behavior of the hook
(veto vs observe).

## Open questions:

- UNVERIFIED: which settings keys gate each visual effect. The render callbacks shown
  here do not expose their tags in the recovered snippets.
- UNVERIFIED: the bodies of the data-slot callbacks (`0x180170FB0`,
  `0x180171FA0`, `0x1801716F0`, `0x180171700`, `0x180169B50`) — they are not split
  into standalone functions, so they must be read from the registrar.
- UNVERIFIED: whether `CCCircleWave::draw` and `GJBaseGameLayer::lightningFlash`
  are feature hooks or incidental registrations.
- UNVERIFIED: whether any of these hooks can veto rendering, or only observe it.

## Evidence — Thread Priority (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18018ADF0` (`0x18018ADF0`) and `FUN_18018AE50` (`0x18018AE50`) implement
Thread Priority:

- `FUN_18018ADF0`: If `isHackEnabled(HACK_THREAD_PRIORITY = 454, 0x1C6)` is
  true, reads tag 0x1C7 (455) via `FUN_18002E4C0` (a uint/int accessor). If the
  value is < 5, sets the current thread's priority to `value - 2` (range -2..2).
  Calls `SetThreadPriority(GetCurrentThread(), priority)`.
- `FUN_18018AE50`: If enabled, takes priority from `*param_2` (if < 5,
  `*param_2 - 2`) and calls `SetThreadPriority`.

Both are vtable entries (0x180831048, 0x180831054), so they are virtual methods
of some class. UNVERIFIED: which class owns the vtable.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1C6 | 454 | `isHackEnabled` (Map B) | `HACK_THREAD_PRIORITY` | Master toggle |
| 0x1C7 | 455 | `FUN_18002E4C0` (uint/int accessor) | (not in static list — runtime-assigned) | Priority level (0–4 → -2..2)

UNVERIFIED: which JSON key binds to tag 0x1C7; the vtable's owning class.

## Evidence — Borderless Fullscreen (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_1801968F0` (`0x1801968F0`) is a vtable entry (0x180831A2C) that, when
tag 0x1EC (492) is enabled, calls `FUN_1801964D0(1)`.

`FUN_1801964D0` (`0x1801964D0`) toggles borderless fullscreen:
1. Gets the current DC's window handle via `wglGetCurrentDC()` → `WindowFromDC()`.
2. Checks `GetWindowLongPtrW(hWnd, GWL_STYLE)` for `WS_CAPTION` (0xCC0000) and
   whether the window has a style at all.
3. If the current border state doesn't match the requested state (`param_1`):
   - Removes window style bits (borderless) or restores them.
   - NOP-patches 6 bytes at a code location (0x90909090 + 0x9090) when enabling,
     restoring the original bytes when disabling.
   - Calls `FUN_1802048F0()` (likely `SetWindowPos` or similar).

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1EC | 492 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Master toggle (Borderless Fullscreen)

UNVERIFIED: which JSON key binds to tag 0x1EC; the vtable's owning class; the
exact GWL style bits modified; the code location being NOP-patched.

## Evidence — Show Triggers (2026-10-25)

`HACK_SHOW_TRIGGERS` (tag 286, `0x11E`; entry #64 in the static label map, store VA
`0x1800640F7`) is registered in `FUN_1800AFAE0` at `0x1800B2CC6`; the callback is
`FUN_180183790`. It installs **three** hooks, which together explain the feature:

| Target | RVA | Detour | Installer |
| --- | ---: | --- | --- |
| `PlayLayer::addObject` | `0x3ADD70` | `0x1801838C0` | `0x18015B2C0` |
| `GJBaseGameLayer::addToSection` | `0x226500` | `0x180183910` | `0x180183AB0` |
| `GameObject::customSetup` | `0x190E20` | `0x180183920` | `0x18017EB00` |

The three detours share one `.data` byte at `0x18080C840`, and it has exactly three
rip-relative references in `.text`, all inside these detours:

```
0x1801838CC  mov  byte [0x18080C840], 0    ; addObject: clear before the original
0x1801838DD  test byte [0x18080C840], 1    ; after it: did the add already run?
0x180183910  mov  byte [0x18080C840], 1    ; addToSection detour
0x180183917  jmp  0x180114EE0              ; ...then continue into the original
```

`GameObject::customSetup` is where the toggle becomes visible: the detour reads the object
ID at `[this+0x40C]`, requires it to be **greater than 0x7DE** (`0x18018392F`), subtracts
`0xB53` and accepts the result when it is `<= 0x86` (`0x180183936`-`0x180183942`), so the
accepted window is the inclusive ID range **0xB53..0xBD9** (2899..3033). Those are 2.2
trigger object IDs, so "Show Triggers" marks trigger objects as they initialise rather than
drawing every object.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x11E | 286 | `isHackEnabled` (Map B) | `HACK_SHOW_TRIGGERS` | Master toggle (Show Triggers) |

UNVERIFIED: what `addObject`'s guarded tail call (`vtable+0x478`, `0x1801838FC`) does; whether
the ID window is the complete trigger range or one branch of a larger test; which JSON key
binds to tag 0x11E; how the marked object is drawn.

## Evidence — Hide UI (2026-10-25)

`HACK_HIDE_UI` (tag 53, `0x35`, static-map entry #26, store VA `0x180063372`) is registered
at `0x1800B0330`; callback `FUN_18015FA70` hooks `EditorUI::init` (RVA `0xDF390`) with detour
`0x18015FB20`, installer `0x18015FB50`.

```
0x18015FB29  call  0x18010FC00             ; gate: returns a bool in al
0x18015FB32  mov   r8, [rsi]               ; vtable
0x18015FB3C  call  qword [r8+0x140]        ; setVisible(false)
```

The detour calls the gate, then calls the object's own vtable slot `+0x140` with `edx = 0`
while preserving the gate's return value in `eax`. That is the same `+0x140` "hide this node"
idiom used by Hide Pause Button and Hide Pause Menu, so the three toggles share one visibility
mechanism even though their targets differ.

Note the hook is on `EditorUI::init` only: the hide is applied once at construction, so the
editor UI stays hidden for that layer's lifetime rather than being re-hidden every frame.

UNVERIFIED: what `FUN_18010FC00` tests and returns (it is a normal function with a frame
prologue, not a thunk); whether the toggle also covers the playtest UI; the JSON key for tag
0x35.

## Evidence — Thread Priority and its v6.2.0 "Switcher" wording (2026-10-26)

Re-read of tag 454 after the static-labelling pass. The tag map and the two bodies in the
2026-10-23 section above are correct; what follows adds the registration chain, confirms the
two imports by their hint/name entries, ties the integer tag 455 to a named settings key, and
establishes that the v6.2.0 changelog wording "Thread Priority Switcher" is the same feature
as the already-mapped v8.0.0-alpha12 wording "Thread Priority".

### Registration chain

`FUN_18018AC90` (code cave, body `0x18018AC90..0x18018AD6F`, `ret` at `0x18018AD6F`) is the
registrar, called from the shared settings initialiser at `0x1800B3231`. For each of the two
tags it builds a `{descriptor, function}` pair on the stack and calls a listener registrar:

| Tag | Registrar call | Function | Descriptor |
| --- | --- | --- | --- |
| `0x1C6` (454) | `0x18018ACE7` -> `0x18008F910` | `0x18018ADF0` | `0x180546628` |
| `0x1C7` (455) | `0x18018AD47` -> `0x180073A70` | `0x18018AE50` | `0x180579678` |

The same two functions are also addressable as 32-bit RVAs at `0x180831048` (`0x0018ADF0`)
and `0x180831054` (`0x0018AE50`) — the pair the earlier note called vtable entries. Both
registrations are therefore real: the class table points at the methods, and the settings
system gets a listener for each tag.

### Bodies

`0x18018ADF0..0x18018AE3A`:

```
0x18018ADF6  call 0x18008A510                 ; settings context
0x18018ADFE  mov edx,0x1C6
0x18018AE06  call 0x18008F420                 ; isHackEnabled(454)
0x18018AE0B  xor edi,edi
0x18018AE0F  je 0x18018AE29                   ; disabled -> priority 0 (normal)
0x18018AE11  mov edx,0x1C7
0x18018AE19  call 0x18002E4C0                 ; integer config, tag 455
0x18018AE1E  lea ecx,[rax-2]
0x18018AE23  cmp eax,5
0x18018AE26  cmovb edi,ecx                    ; value<5 ? value-2 : 0
0x18018AE29  call [0x180642F00]               ; GetCurrentThread
0x18018AE32  mov edx,edi
0x18018AE3A  jmp [0x1806430B0]                ; SetThreadPriority
```

`0x18018AE50..0x18018AE98` is the value-listener form: it reads the same master tag, takes
the new level from `dword [rdx]` instead of re-reading tag 455, applies the same
`value < 5 ? value-2 : 0` mapping and calls `SetThreadPriority(GetCurrentThread(), level)`;
when disabled it returns without touching the thread (`0x18018AE92`).

Both import slots were confirmed from their import hint/name entries rather than by name
inference: slot `0x180642F00` -> hint `0x245` "GetCurrentThread", slot `0x1806430B0` ->
hint `0x5A7` "SetThreadPriority".

### Settings keys

`notes/settings-contents.md` line 218 `UNIVERSAL/THREAD_PRIORITY/COMBO` = 4 and line 305
`UNIVERSAL/THREAD_PRIORITY/HACK_THREAD_PRIORITY`. The `COMBO` entry is the priority selector
the v6.2.0 changelog calls the "Switcher", and its default 4 maps to Windows priority 2
(`ABOVE_NORMAL`) under the `value - 2` rule.

### Why one tag serves both changelog wordings

There is exactly one static label `HACK_THREAD_PRIORITY` (static-map entry 73, store
`0x180064483`), one registrar, one master tag and one level tag in the image. The v6.2.0
"Thread Priority Switcher" row and the v8.0.0-alpha12 "Thread Priority" row therefore
describe the same toggle-plus-combo pair; the earlier wording is promoted on that shared
mechanism, not on a second implementation that would have to be found.

Method caution: a raw scan for `mov edx,0x1C7` also hits statically linked OpenSSL at
`0x180404D59` (`crypto\property\property_parse.c`, `ossl_parse_query`). Tag scans must
require the adjacent config-accessor call, not just the immediate.

UNVERIFIED: which thread the listener runs on (the call is on the thread that fires the
settings change); whether `COMBO` stores 0-4 or an offset form; what the game does for a
stored value >= 5 (the code falls back to normal priority 0); and whether the v6.2.0 build
exposed the selector with the same key path.

## Evidence - No Portal Circle and No Ghost Trail (2026-10-26)

Two v9.0.x cosmetic rows are single-target suppressions in the runtime-assigned tag block.
Both hook a host method whose friendly name states the feature, and both detours are one
byte long, so the mechanism is the same suppression idiom already established for No Orb
Ring (`0x1A4`) and No Spider Dash (`0x1B2`).

| Field | No Portal Circle | No Ghost Trail |
| --- | --- | --- |
| Changelog row | v9.0.0 "Added No Portal Circle" | v9.0.2 "Added No Ghost Trail" |
| Tag | 421 (`0x1A5`, no static label) | 432 (`0x1B0`, no static label) |
| Registration | `0x1800B130D` (`ba a5 01 00 00`) | `0x1800B0F55` (`ba b0 01 00 00`) |
| Register call | `0x1800B1318` `call 0x18008F910` | `0x1800B0F60` `call 0x18008F910` |
| Callback | `FUN_180171350` | `FUN_180169AE0` |
| Callback body | `0x180171350..0x1801713A8` (`ret` at `0x1801713A8`) | `0x180169AE0..0x180169B39` |
| Disable path | `0x1801713A9` `jmp 0x180108A80` with the detour pointer | same shape |
| Target | `PlayerObject::spawnPortalCircle`, RVA `0x397760` | `PlayerObject::toggleGhostEffect`, RVA `0x39EE30` |
| Name string | `0x1805758DE` "PlayerObject::spawnPortalCircle" | `0x1805454A6` "PlayerObject::toggleGhostEffect" |
| Detour | `0x1801713C0` | `0x180169B50` |
| Detour bytes | `c3` then `cc` padding | `c3` then `cc` padding |
| Installer | `0x180168B00`, args `[rsp+0x20]=0xFFFFF428`, `[rsp+0x28]=1` | `0x1800AB3D0`, same args |

Both callbacks share the standard enable/disable shape: `movzx ebx,[rdx]` reads the new
enabled state, `0x180108960` fetches the hook context, `cmp bl,1` selects install versus
`0x180108A80` (uninstall), and the install branch resolves the host RVA through
`0x18010A250` -> `0x18010A370` before passing the detour pointer in `r8`.

### Why a one-byte detour is the whole feature

The detour is the function the hook replaces the host prologue with. `0x1801713C0` and
`0x180169B50` are each a single `ret` (`c3`) followed by `cc` int3 padding, so the hooked
routine returns immediately and none of `spawnPortalCircle` / `toggleGhostEffect` executes.
A detour that wants the original to run instead resumes it explicitly, and this image does
that through `0x1801A3D20` - the NoClip `destroyPlayer` detour tail-jumps there at
`0x18017A403`, and the tag-437 `destroyPlayer` detour calls it at `0x180169F33`. Neither
of these two detours contains a call or a jump at all.

Corroboration that this is the suppression convention and not an artifact: tag 437
(`0x1B5`, registration `0x1800B0D02`, callback `FUN_1801684B0`) hooks eight effect routines,
and its `spawnPortalCircle` detour `0x1801687F0` is also a bare `ret`, as is its partner on
`RingObject::spawnCircle`. Independent features reusing the identical one-byte body is a
second angle on the reading.

### Binding 421 to "No Portal Circle" over the v9.0.7 row

`0x1A5` was previously left unpromoted because "Added No Circle Effect" (v9.0.7) also
describes a portal circle and only one tag hooks `spawnPortalCircle`. Two observations
now separate them:

1. The runtime-assigned tags are allocated in release order. Using only bindings that are
   already mapped: 420 No Orb Ring (v9.0.0), 429/430 Accurate Percentage (v9.0.0),
   434 No Spider Dash (v9.0.4), 440/441 No Robot/Swing Fire (v9.0.8). Tag 421 falls inside
   the v9.0.0 group, next to 420 (`RingObject::spawnCircle`) and the flip-portal suppression
   at `0x1A1`; tag 432 falls between the v9.0.0 group and 434, which is where a v9.0.2
   addition belongs. A v9.0.7 feature therefore has to sit at the 437 end of the range,
   not at 421.
2. Tag 437 hooks three circle spawners in one batch (`spawnPortalCircle`,
   `spawnDualCircle` RVA `0x397B70`, `CCCircleWave::create` RVA `0x42870`) plus five other
   player effects. That is the shape of a multi-target "no circle effect" toggle, whereas
   421 is a single-target portal-circle suppression.

**Inference, not proof:** rule 1 depends on the allocator being monotone, which the image
does not state, so the v9.0.0 row is promoted on the target name plus the allocation
argument, with that reasoning recorded rather than hidden; if 437 later resolves to a
different wording, this binding should be re-checked first.

Resolved for tag 437 (2026-10-26): rule 2 is no longer a shape argument. The five "other"
targets are not generic effects - each is wrapped only so that a circle wave created inside
it is hidden, and `CCCircleWave::create` is the single reader of that scoped flag. Every one
of the eight targets therefore bears on circle waves, which is what "No Circle Effect"
describes. The v9.0.7 row is now `mapped`; see the section at the end of this note.

### Cancelled lead: No New Best Popup

`0x180169EB0` looked like a candidate third row because it hooks `PlayLayer::showNewBest`
(RVA `0x3A9550`, name string `0x1805759E9`) and returns immediately under a tag check. The
tag it reads is `0xF5` = 245 = `HACK_RESPAWN_TIME`, and the same callback `FUN_180169DE0`
also hooks `PlayLayer::destroyPlayer`, sets `[0x18080C378]=1` on `showNewBest` and clears it
in the destroy detour. So the pair tracks level completion for the respawn timer; it is not
a popup suppression. "Added No New Best Popup" stays `cluster`, and the reason is now
recorded instead of re-investigated.

UNVERIFIED: the settings keys that write tags 421 and 432 (neither has a static label, so
neither appears in [settings-contents.md](settings-contents.md)); the runtime effect, since
no Geometry Dash session was run; whether the hook manager keeps two detours on one target,
which matters if tag 437 and tag 421 are enabled together; and the meaning of the installer
argument `0xFFFFF428`, used by many hooks in this image.

## Evidence - Frozen Animations and No Dash Fire (2026-10-26)

Two more single-purpose entries in the runtime-assigned cosmetic block. Both replace a host
routine outright, and in both cases the replacement body is small enough that the mechanism
can be read off the instructions alone.

| Field | Frozen Animations | No Dash Fire |
| --- | --- | --- |
| Changelog row | v9.0.2 "Added Frozen Animations" | v9.0.4 "Added No Dash Fire" |
| Tag | 433 (`0x1B1`, no static label) | 436 (`0x1B4`, no static label) |
| Registration | `0x1800B0A38` (`ba b1 01 00 00`) | `0x1800B0D79` (`ba b4 01 00 00`) |
| Callback | `FUN_180167490` | `FUN_180169260` |
| Target 1 | `CCAnimatedSprite::runAnimation`, RVA `0x40CF0` | `PlayerObject::updateDashAnimation`, RVA `0x396D20` |
| Target 2 | `CCAnimatedSprite::tweenToAnimation`, RVA `0x40D60` | - |
| Name strings | `0x180575800`, `0x18057581F` | `0x1805454C6` |
| Detours | `0x180167560`, `0x1801675C0` | `0x1801692D0` |
| Installers | `0x180167620`, `0x180167800` | `0x1800AB230` |
| Installer flags | `[rsp+0x20]=0xFFFFF428`, `[rsp+0x28]=1` | same |

### Frozen Animations is an argument-only body

`0x180167560` and `0x1801675C0` are byte-identical in shape and 0x26 bytes long:

| Site | Instruction | Reading |
| --- | --- | --- |
| `0x180167565` | `mov rsi, rdx` | second argument = the by-value `std::string` name (MSVC passes a 0x20-byte struct by hidden pointer) |
| `0x180167568` | `mov rax, [rdx+0x18]` | string capacity |
| `0x18016756C` | `cmp rax, 0x10` / `jb` | the SSO threshold: capacity >= 0x10 means a heap buffer |
| `0x18016759D` | `call 0x1803A50F0` | free the heap buffer |
| `0x1801675A2` | `[rsi+0x10]=0`, `[rsi+0x18]=0xF`, `byte [rsi]=0` | reset to the empty small-string form |
| `0x1801675BA` | `ret` | no call, no jump to the original |

So the hooked routine does nothing except destroy its own by-value argument, which is exactly
the cleanup a callee owes the caller and nothing else. Clearing the name is not a filter that
one animation could still pass: the call never reaches gameplay code, so no animation starts
and an animated sprite keeps whatever frame it already had. Both entry points are replaced,
so neither `runAnimation` nor `tweenToAnimation` can revive it.

### No Dash Fire hides the node and skips the update

`0x1801692D0` takes `rcx` = the `PlayerObject`, loads `[this+0x660]`, and returns if it is
null. Otherwise it calls vtable slot `+0x140` of that node with `edx=0` - the same shared
show/hide slot used by Hide UI, the pause-button detours and the NoClip tint - then loads the
sub-object at `node+0x140`, takes its vtable slot `+0x28` and tail-jumps it with `edx=0`.
The original `updateDashAnimation` is never called. The player itself is untouched, so what
disappears is the child effect the dash animation would otherwise drive; the row's wording
calls that the dash fire.

### Binding

| Angle | Frozen Animations | No Dash Fire |
| --- | --- | --- |
| Host name | `CCAnimatedSprite::runAnimation` / `tweenToAnimation` - every animated sprite in the game | `PlayerObject::updateDashAnimation` - the routine that drives the dash effect |
| Tag order | 433 sits between 432 No Ghost Trail (v9.0.2) and 434 No Spider Dash (v9.0.4) | 436 sits in the same v9.0.4 group as 434 |
| Settings key | none observed: no frozen-animation key appears in [settings-contents.md](settings-contents.md) | none observed: only `COSMETIC/HACK_NO_SPIDER_DASH` exists for the dash family |

The tag-order column is the same inference used for No Portal Circle: the runtime-assigned
tags follow release order for every row that is mapped so far. It is corroboration, not
proof.

UNVERIFIED: the settings keys that write tags 433 and 436; that `[this+0x660]` is the fire
sprite specifically rather than the whole dash-effect node (the instructions only prove that
a child of the player is hidden inside the dash-animation update); whether clearing the
animation name also stops an already-running animation, since the code only prevents new
calls; and the runtime effect in a live session.

Correction (2026-10-26): the tag-435 continuation has now been read, and it changes the
reading recorded above. `0x180169EB0` sets `byte [0x18080C378]=1`, returns early when tag 245
is enabled, and otherwise reads host setting `"0052"` and tail-jumps to `0x18017F000` with
`xmm1` = 1.0 (`0x180534360`) or 0.5 (`0x180534338`). `0x18017F000` reads tags 502 (`0x1F6`,
bool master), 247 (`0xF7`, bool) and 501 (`0x1F5`, double), calls
`CCNode::stopActionByTag(0x10)` and clamps a delay with `maxsd`. Tag 245 is not a guess:
`notes/static-hack-label-map.md` recovers the real `{label -> uint64}` table built by
`FUN_180062940`, so `HACK_RESPAWN_TIME = 245` is the value the profile loader itself writes,
and tag 245 is read at exactly two sites in the whole image, both inside this popup path. The
pair therefore implements "Respawn Time replaces the stock new-best popup" plus a rescheduled
popup driven by tags 501/502/247, not a No New Best Popup toggle; tag 435 is the mod's own
popup scheduler. The row stays `cluster` because no changelog wording describes a rescheduled
popup, and the "no new best popup" wording has no located toggle.

## Evidence - No End Shake (2026-10-26)

`Added No End Shake` (v9.0.8) is the only v9.0.x row about the end-of-level effect, and it has
a matching settings key, a tag, and a detour whose whole body is a single field write.

| Field | Value |
| --- | --- |
| Row | v9.0.8 `Added No End Shake` (`notes/feature-inventory.md`) |
| Settings key | `COSMETIC/HACK_NO_END_SHAKE` (`notes/settings-contents.md`) |
| Tag | 439 (`0x1B7`) - unlabelled; absent from the 85-entry static map, which is a real `{label -> uint64}` table, so the absence is meaningful |
| Registration | `0x1800B0EDE`, bytes `ba b7 01 00 00` = `mov edx,0x1B7` |
| Callback | `FUN_180169A40`, body `0x180169A40..0x180169A98`; enable path resolves the host RVA, disable path `0x180169A99` tail-jumps `0x180108A80` |
| Target | `PlayLayer::showCompleteEffect`, RVA `0x3A8F30` (host module), name string `0x1805759AC` |
| Installer | `0x1800AB850`, `[rsp+0x20]=0xFFFFFFE0`, `[rsp+0x28]=1` |
| Detour | `0x180169AB0`, 0x1D bytes |

The detour, verbatim:

```
0x180169AB0  push rsi
0x180169AB1  sub  rsp, 0x20
0x180169AB5  mov  rsi, rcx
0x180169AB8  call 0x1801A5AC0
0x180169ABD  mov  dword ptr [rsi + 0x520], 0
0x180169AC7  add  rsp, 0x20
0x180169ACB  pop  rsi
0x180169ACC  ret
```

`0x1801A5AC0` is the delayed-bind trampoline for RVA `0x3A8F30`: it guards on
`0x1808100C0`, calls `0x18010DDC0` for the host module base at `0x1801A5B1F`, forms
`lea rcx,[rax+0x3A8F30]` at `0x1801A5B24`, resolves with `0x1801A33C0` and tail-jumps through
the cache at `0x1808100B8`. So the original `showCompleteEffect` always runs; the mod only adds
the field write afterwards.

### Why the field write is the whole feature

A `+0x520` displacement scan of the entire `.text` finds 52 memory operands, and exactly one of
them has a base other than `rsp`/`rbp` - this site (`c7 86 20 05 00 00 00 00 00 00`). No other
mod code reads or writes that PlayLayer field, so the toggle's only observable effect is
zeroing it immediately after the completion effect arms it.

UNVERIFIED: the field's identity. The inference is that `[PlayLayer+0x520]` carries the
end-of-level shake state (amplitude or timer) that `showCompleteEffect` sets, so zeroing it in
the same frame cancels the shake; "shake" is the only end-of-level cosmetic the v9.0.8 batch
names and `COSMETIC/HACK_NO_END_SHAKE` is the only matching key. The alternative reading is
that the dword is the completion flash/tint state, which would make the row a "no end flash".
Settling it needs a runtime observation of the field across the completion frame, or a GD
2.2081 binding/header; the sample set holds only the mod's DLL and SO builds, so the host
routine `0x3A8F30` cannot be disassembled from here.

Corroboration for the era: tag 439 sits between 436 (`No Dash Fire`, v9.0.4) and 440/441
(`No Robot/Swing Fire`, v9.0.8), which is where a v9.0.8 addition belongs under the release
order that holds for every mapped row. It is also a different tag with a different target from
`No Shake` (409), which instead scopes host setting `"0172"` around `PlayLayer::init`; see
[no-shake-feature.md](no-shake-feature.md). That note's open question about whether
`No End Shake` is a separate feature or a refinement resolves to **separate**, with the field
identity still open.

## Evidence - No Circle Effect (2026-10-26)

Tag 437 (`0x1B5`) installs eight detours in one callback, and together they suppress the
circular wave effect from every direction: two spawners are disabled outright, five routines
are wrapped so that any wave they create is hidden, and the wave factory itself is the ninth
install (the filter). The settings key is absent, but the mechanism is closed enough to name
the row.

| Field | Value |
| --- | --- |
| Row | v9.0.7 `Added No Circle Effect` |
| Tag | 437 (`0x1B5`), no static label |
| Registration | `0x1800B0D02`, bytes `ba b5 01 00 00` = `mov edx,0x1B5` |
| Callback | `FUN_1801684B0`, body `0x1801684B0..0x180168791`; one install + one disable branch per target, disable path tail-jumps `0x180108A80` |
| Scoped flag | `byte [0x18080C2F0]` (`.bss`), written only by these detours |

### The eight installs

Each row was confirmed three ways: the `mov edx,<RVA>` in the callback, the name string
passed in `r9`, and the delayed-bind trampoline's `lea rcx,[rax+RVA]`.

| # | Target RVA | Host method (string VA) | Detour | Installer | Behaviour |
| --- | --- | --- | --- | --- | --- |
| 1 | `0x39FA50` | `PlayerObject::playBumpEffect` (`0x1805758C1`) | `0x1801687B0` | `0x1801688B0` | flag=1, trampoline `0x1801A7CF0`, flag=0 |
| 2 | `0x398C00` | `PlayerObject::ringJump` (`0x1805756F5`) | `0x1801687D0` | `0x180165DC0` | flag=1, trampoline `0x1801A89F0`, flag=0 |
| 3 | `0x397760` | `PlayerObject::spawnPortalCircle` (`0x1805758DE`) | `0x1801687F0` | `0x180168B00` | `c3` - bare `ret`, nothing spawns |
| 4 | `0x397B70` | `PlayerObject::spawnDualCircle` (`0x1805758FE`) | `0x180168800` | `0x1800AB230` | `c3` - bare `ret` |
| 5 | `0x39AF90` | `PlayerObject::toggleDartMode` (`0x1805453F6`) | `0x180168810` | `0x1800AAFE0` | flag=1, trampoline `0x1801A9310`, flag=0 |
| 6 | `0x3A0700` | `PlayerObject::togglePlayerScale` (`0x18057591C`) | `0x180168830` | `0x1800AAFE0` | flag=1, trampoline `0x1801A9760`, flag=0 |
| 7 | `0x1AA4A0` | `GameObject::playDestroyObjectAnim` (`0x18057593C`) | `0x180168850` | `0x180168D50` | flag=1, trampoline `0x18014A950`, flag=0 |
| 8 | `0x42870` | `CCCircleWave::create` (`0x18057595E`) | `0x180168870` | `0x180168F30` | calls the factory, then hides the result while the flag is up |

Argument slots: `[rsp+0x28]=1` for all eight; `[rsp+0x20]=0xFFFFF428` for the two circle
spawners (#3, #4) and `0xFFFFFFE0` for the rest.

### The flag, and why it closes the case

`byte [0x18080C2F0]` has exactly eleven references in `.text` - ten writes (set before and
cleared after each of the five wrapped calls, rows 1, 2, 5, 6, 7) and **one** read, at
`0x180168883` in the `CCCircleWave::create` detour:

```
0x180168870  push rsi
0x180168871  sub  rsp, 0x30
0x180168875  movzx eax, byte ptr [rsp + 0x60]     ; forward the 5th stack argument
0x18016887A  mov  byte ptr [rsp + 0x20], al
0x18016887E  call 0x1801E5A80                     ; real CCCircleWave::create (RVA 0x42870)
0x180168883  cmp  byte ptr [rip + ...], 1         ; -> 0x18080C2F0
0x18016888A  jne  0x1801688A1
0x18016888C  mov  r8, qword ptr [rax]             ; vtable
0x18016888F  mov  rcx, rax
0x180168892  xor  edx, edx                        ; edx = 0
0x180168894  mov  rsi, rax
0x180168897  call qword ptr [r8 + 0x140]          ; shared visibility setter -> hide
0x18016889E  mov  rax, rsi                        ; still return the node
0x1801688A1  add  rsp, 0x30
0x1801688A5  pop  rsi
0x1801688A6  ret
```

`0x1801E5A80` is the delayed-bind trampoline for RVA `0x42870` (`lea rcx,[rax+0x42870]` at
`0x1801E5B12`), so the real factory always runs and the mod only hides its product. The
vtable slot `+0x140` is the same visibility setter used by Hide UI, the pause-button detours,
the NoClip tint and the No Dash Fire detour - here called with `edx=0`.

So the five wrapped routines are not "other effects" that happen to be in the batch: each is a
moment at which the game creates a circle wave, and the flag is the marker that says "hide the
wave this call produces". Nothing else in the image reads that flag.

Corroboration for the era: 437 sits between 436 (`No Dash Fire`, v9.0.4) and 439
(`No End Shake`, v9.0.8), so it belongs to the v9.0.4-v9.0.8 window where the v9.0.7 row
lives; the eight targets are all circular-wave producers.

UNVERIFIED: whether the older v7.1 wording `Added No Effect Circle` is the same tag (see
[feature-inventory.md](feature-inventory.md) - only one unassigned circle-suppression tag
exists, so the image cannot separate a rename from a duplicate); that `toggleDartMode`,
`togglePlayerScale` and `playDestroyObjectAnim` call `CCCircleWave::create` internally
(inferred from the scoped-flag shape plus the single read site, not observed); and the runtime
appearance, since no session was run.
