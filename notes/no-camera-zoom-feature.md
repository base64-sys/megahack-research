# No Camera Zoom feature

- **Question** — which code implements the "No Camera Zoom" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_CAMERA_ZOOM` (tag 388, `0x184`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b0c8b`:

```
0x1800b0c8b  mov  edx, 0x184          ; tag = HACK_NO_CAMERA_ZOOM
0x1800b0c90  mov  rcx, rax            ; resolver
0x1800b0c93  mov  r8, rsi             ; context structure
0x1800b0c96  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801682c0` (`0x1801682c0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `EffectGameObject::triggerObject`
  (RVA `0x4A5F30`) via `FUN_180168350`, using the string
  `"EffectGameObject::triggerObject"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x184 | 388 | (registered via FUN_18008f910) | `HACK_NO_CAMERA_ZOOM` | Master toggle (No Camera Zoom) |

## Method

1. Searched for byte pattern `ba 84 01 00 00` (mov edx, 0x184) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b0c8b`).
2. Disassembled `FUN_1800afae0` around `0x1800b0c8b` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801682c0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `EffectGameObject::triggerObject` (RVA `0x4A5F30`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x184.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes trigger events).