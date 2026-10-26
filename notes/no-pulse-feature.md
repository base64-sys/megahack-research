# No Pulse feature

- **Question** — which code implements the "No Pulse" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_PULSE` (tag 413, `0x19D`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b1384`:

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

## Method

1. Searched for byte pattern `ba 9d 01 00 00` (mov edx, 0x19D) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b1384`).
2. Disassembled `FUN_1800afae0` around `0x1800b1384` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801713e0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `PlayLayer::updateVisibility` (RVA `0x3AF3A0`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x19D.
- ~~UNVERIFIED: the exact behavior of the hook~~ — answered 2026-10-26: the detour writes
  three 0.5f constants and then runs the vanilla function. See the evidence section below.

## Evidence — No Pulse detour body (2026-10-26)

The open question above is now answered: the hook does not veto `updateVisibility`, it
rewrites three pulse-driven floats to 0.5 on every visibility pass. Read from
`absolllute.hackmega_original.dll` (SHA-256
`1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`) with capstone over the
file-backed range; the detour is a code cave with no `.pdata` record.

| Field | Value |
| --- | --- |
| Tag | 413 (`0x19D`, `HACK_NO_PULSE`), static map entry 42, store `0x1800638FC` |
| Registration | `0x1800B1384` in `FUN_1800AFAE0` (`mov edx,0x19D`) |
| Callback | `0x1801713E0` (install order `0xFFFFFFE0`, sixth argument 1) |
| Installer | `0x1800AB9F0` |
| Target | `PlayLayer::updateVisibility`, RVA `0x3AF3A0` |
| Detour | `0x180171450`, body `0x180171450..0x1801714A1`, `int3` padding after |

Body, in order:

```
0x180171450  (prologue)            ; saves nonvolatiles, xmm6 <- xmm1
0x1801714??  mov rsi, rcx          ; this = PlayLayer
0x1801714??  mov rax, [rcx+0x3270]
0x1801714??  mov dword [rax+0x23C], 0x3F000000   ; 0.5f
0x1801714??  call 0x180128CF0
0x1801714??  mov dword [rax+0x210], 0x3F000000   ; 0.5f
0x1801714??  mov rax, [rsi+0x3270]
0x1801714??  mov dword [rax+0x244], 0x3F000000   ; 0.5f
0x1801714??  (restore xmm1/xmm6)
0x1801714??  jmp 0x1801A3A70       ; cached original updateVisibility
```

The tail jump is the delayed-bind trampoline for the same target it hooks: its initialiser
does `lea rcx,[rax+0x3AF3A0]` at `0x1801A3AE5`, stores the resolved pointer at
`0x1801A3AF1` (cache `0x180807CD8`), and uses builder `0x1801A3890` with the module base from
`0x18010DDC0`. So the detour writes the constants **and then** runs the vanilla function,
which is why the values observed by the original are already pinned — the writes are not a
post-pass correction.

Interpretation: three floats that would otherwise animate at the frame rate are pinned to
the neutral 0.5 value every frame the layer updates visibility. `[layer+0x3270]` is the same
object that carries `+0x23C` and `+0x244`; `0x180128CF0` returns the second object whose
`+0x210` float is pinned by the same call. `0x180128CF0` is the getter Auto Music Sync used
to reach `[engine+0x228]`, so the three fields plausibly live on the same audio/visual
engine grouping, but no name string identifies any of them.

### Ruled out for Classic Pulse

`notes/render-hooks.md` also lists a second `PlayLayer::updateVisibility` hook (callback
`0x1801582C0`, detour `0x180158740`) and an editor counterpart (`0x180158830`). Both were
read this run and are the **hitbox** pass, not a pulse-rate override:

- `0x180158740` first calls the cached original through `0x1801A3A70`, then `0x1802022C0`,
  shows `[layer+0x3770]` with `[vtable+0x140]` called with `1`, looks up children `0x1E6813A`,
  `0x1E6813B`, `0x1E6813C` with `[vtable+0x1C8]` and routes them to `0x180203F60`
  (hitbox draw), `0x1801809A0` (hitbox trail) and `0x180182520`.
- Its two call sites (`0x18015834D`, `0x180158416`) are inside one callback body that also
  installs the editor hook, which is the Show Hitboxes / Hitbox Trail family.

No 1/60-second or 60.0 pulse constant exists in the image (a `.text` scan for `0x3C888889`,
`0x3D088889`, `0x42700000` and `0x42F00000` found only the Smooth Editor Trail 1/30 store,
a 60.0 float at `0x180198436`, and two 1/30 sites at `0x18023E88D`/`0x18023E912`). There is
no `HACK_CLASSIC_PULSE` entry in the 85-label static map, so the toggle is runtime-assigned
and cannot be tied from the label side either.

UNVERIFIED: the names and owners of `[layer+0x3270]+0x23C`/`+0x244` and of the
`0x180128CF0` object's `+0x210`; which JSON key binds tag 413 (the settings tree has
`COSMETIC/HACK_NO_PULSE`, but no site reading that key next to the tag was found); and the
implementation of Classic Pulse, which remains unlocated — it is neither the 0x19D detour nor
the 0x1801582C0 hitbox hook.
