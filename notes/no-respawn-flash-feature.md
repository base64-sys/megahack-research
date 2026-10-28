# No Respawn Flash feature

- **Question** — which code implements the "No Respawn Flash" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_RESPAWN_FLASH` (tag 398, `0x18E`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b13fb`:

```
0x1800b13fb  mov  edx, 0x18E          ; tag = HACK_NO_RESPAWN_FLASH
0x1800b1400  mov  rcx, rax            ; resolver
0x1800b1403  mov  r8, rsi             ; context structure
0x1800b1406  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801714c0` (`0x1801714c0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayerObject::playSpawnEffect`
  (RVA `0x397480`) via `FUN_1800AB230`, using the string
  `"PlayerObject::playSpawnEffect"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108A80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x18E | 398 | (registered via FUN_18008f910) | `HACK_NO_RESPAWN_FLASH` | Master toggle (No Respawn Flash) |

## Method

1. Searched for byte pattern `ba 8e 01 00 00` (mov edx, 0x18E) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b13fb`).
2. Disassembled `FUN_1800afae0` around `0x1800b13fb` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801714c0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `PlayerObject::playSpawnEffect` (RVA `0x397480`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x18E.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes spawn effects).

## Changelog wordings and the detour byte (2026-10-28)

Two changelog lines name this one tag: v6.2.0 (Full) "Added No Respawn Flash mod" and
v8.0.0-alpha5 "Added No Respawn Flash". There is no second mechanism to find - tag 398 has
exactly one registration site.

**Boundary.** A whole-`.text` scan for `ba 8e 01 00 00` (`mov edx,0x18E`) returns one site,
`0x1800B13FB`, which is the registration above; nothing else reads tag 398 by immediate.

**Detour.** The installed detour is `0x180171530` and it is a single byte:

```
180171530  c3    ret
180171531  cc cc cc ...        ; int3 padding to the next .pdata record
```

(`pe_disasm.py absolllute.hackmega_original.dll 0x180171530 --size 40`, run from
`samples/absolllute.hackmega`; VA `0x180171530`, RVA `0x171530`, file `0x170930`.) The body
never reaches the cached original, so every `PlayerObject::playSpawnEffect` call is a no-op
while the toggle is on. The registration passes order `0xFFFFF428` (-3032) and the final
argument `1`.

UNVERIFIED: whether `playSpawnEffect` is only the spawn ring (the name says so, and no other
caller of the detour exists), and what the host draws for a respawn when the routine is
suppressed; closing either needs a runtime capture, not the image.
