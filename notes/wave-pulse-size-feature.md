# Wave Pulse Size feature

- **Question** — which code implements the "Wave Pulse Size" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_WAVE_PULSE_SIZE` (tag 407, `0x197`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b1c59`:

```
0x1800b1c59  mov  edx, 0x197          ; tag = HACK_WAVE_PULSE_SIZE
0x1800b1c5e  mov  rcx, rax            ; resolver
0x1800b1c61  mov  r8, rsi             ; context structure
0x1800b1c64  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180173920` (`0x180173920`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayerObject::update`
  (RVA `0x388D80`) via `FUN_1800AAC20`, using callback `FUN_180173990`
  and the string `"PlayerObject::update"` with hook priority `0xFFFFF810` (-2032).
- If `*param_2 != 1`: unregisters the hook via `FUN_180108A80()`.

Note: `HACK_NO_WAVE_PULSE` (tag 394, `0x18A`) also hooks `PlayerObject::update`
but with a different callback (`LAB_1801721D0`) and priority `0xFFFFFFE0` (-32).

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x197 | 407 | (registered via FUN_18008f910) | `HACK_WAVE_PULSE_SIZE` | Master toggle (Wave Pulse Size) |

## Method

1. Searched for byte pattern `ba 97 01 00 00` (mov edx, 0x197) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b1c59`).
2. Disassembled `FUN_1800afae0` around `0x1800b1c59` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180173920` (the callback) with REA/Ghidra 12.1.2 — hooks
   `PlayerObject::update` (RVA `0x388D80`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x197.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes player updates).
- UNVERIFIED: how the hook modifies wave pulse size (the callback `FUN_180173990`
  is not yet analyzed).