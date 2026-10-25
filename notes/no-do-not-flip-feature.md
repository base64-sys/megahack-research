# No "Do Not" Flip feature

- **Question** — which code implements the "No 'Do Not' Flip" gameplay toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_DO_NOT_FLIP` (tag 412, `0x19C`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b0e67`:

```
0x1800b0e67  mov  edx, 0x19C          ; tag = HACK_NO_DO_NOT_FLIP
0x1800b0e6c  mov  rcx, rax            ; resolver
0x1800b0e6f  mov  r8, rsi             ; context structure
0x1800b0e72  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801696e0` (`0x1801696e0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers hooks on two GD methods:
  1. `EndLevelLayer::customSetup` (RVA `0x132E50`) via `FUN_180169890`
  2. `PlayLayer::setupHasCompleted` (RVA `0x3A66D0`) via `FUN_1800AB850`
- If `*param_2 != 1`: unregisters both hooks via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x19C | 412 | (registered via FUN_18008f910) | `HACK_NO_DO_NOT_FLIP` | Master toggle (No "Do Not" Flip) |

## Method

1. Searched for byte pattern `ba 9c 01 00 00` (mov edx, 0x19C) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b0e67`).
2. Disassembled `FUN_1800afae0` around `0x1800b0e67` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801696e0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `EndLevelLayer::customSetup` (RVA `0x132E50`) and `PlayLayer::setupHasCompleted`
   (RVA `0x3A66D0`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x19C.
- UNVERIFIED: the exact behavior of the hooks (whether they veto or observe).
- UNVERIFIED: how `EndLevelLayer::customSetup` and `PlayLayer::setupHasCompleted`
  relate to the "Do Not" flip feature.