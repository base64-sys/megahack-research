# No Wave Trail feature

- **Question** — which code implements the "No Wave Trail" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_WAVE_TRAIL` (tag 397, `0x18D`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b17b3`:

```
0x1800b17b3  mov  edx, 0x18D          ; tag = HACK_NO_WAVE_TRAIL
0x1800b17b8  mov  rcx, rax            ; resolver
0x1800b17bb  mov  r8, rsi             ; context structure
0x1800b17be  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801721f0` (`0x1801721f0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `HardStreak::updateStroke`
  (RVA `0x2B8EF0`) via `FUN_180167020`, using the string
  `"HardStreak::updateStroke"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x18D | 397 | (registered via FUN_18008f910) | `HACK_NO_WAVE_TRAIL` | Master toggle (No Wave Trail) |

## Method

1. Searched for byte pattern `ba 8d 01 00 00` (mov edx, 0x18D) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b17b3`).
2. Disassembled `FUN_1800afae0` around `0x1800b17b3` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801721f0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `HardStreak::updateStroke` (RVA `0x2B8EF0`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x18D.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes streak updates).