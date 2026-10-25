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
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes visibility updates).