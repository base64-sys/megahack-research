# No Camera feature

- **Question** — which code implements the "No Camera" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_CAMERA` (tag 387, `0x183`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b0c14`:

```
0x1800b0c14  mov  edx, 0x183          ; tag = HACK_NO_CAMERA
0x1800b0c19  mov  rcx, rax            ; resolver
0x1800b0c1c  mov  r8, rsi             ; context structure
0x1800b0c1f  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801680e0` (`0x1801680e0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `CameraTriggerGameObject::triggerObject`
  (RVA `0x4BD820`) via `FUN_180168160`, using the string
  `"CameraTriggerGameObject::triggerObject"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x183 | 387 | (registered via FUN_18008f910) | `HACK_NO_CAMERA` | Master toggle (No Camera) |

## Method

1. Searched for byte pattern `ba 83 01 00 00` (mov edx, 0x183) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b0c14`).
2. Disassembled `FUN_1800afae0` around `0x1800b0c14` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801680e0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `CameraTriggerGameObject::triggerObject` (RVA `0x4BD820`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x183.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes camera triggers).