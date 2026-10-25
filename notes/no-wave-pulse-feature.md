# No Wave Pulse feature

- **Question** — which code implements the "No Wave Pulse" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_WAVE_PULSE` (tag 394, `0x18A`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b173c`:

```
0x1800b173c  mov  edx, 0x18A          ; tag = HACK_NO_WAVE_PULSE
0x1800b1741  mov  rcx, rax            ; resolver
0x1800b1744  mov  r8, rsi             ; context structure
0x1800b1747  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180172160` (`0x180172160`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayerObject::update`
  (RVA `0x388D80`) via `FUN_1800AAC20`, using the label `LAB_1801721D0`
  and the string `"PlayerObject::update"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108A80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x18A | 394 | (registered via FUN_18008f910) | `HACK_NO_WAVE_PULSE` | Master toggle (No Wave Pulse) |

## Method

1. Searched for byte pattern `ba 8a 01 00 00` (mov edx, 0x18A) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b173c`).
2. Disassembled `FUN_1800afae0` around `0x1800b173c` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180172160` (the callback) with REA/Ghidra 12.1.2 — hooks
   `PlayerObject::update` (RVA `0x388D80`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x18A.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes player updates).
- UNVERIFIED: how PlayerObject::update relates to wave pulse rendering.