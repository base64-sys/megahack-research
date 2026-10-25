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
