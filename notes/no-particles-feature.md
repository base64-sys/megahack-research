# No Particles feature

- **Question** - which code implements the "No Particles" cosmetic toggle?
- **Baseline** - Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** - `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_PARTICLES` (tag 402, `0x192`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b11a8`:

```
0x1800b11a8  mov  edx, 0x192          ; tag = HACK_NO_PARTICLES
0x1800b11ad  mov  rcx, rax            ; resolver (from FUN_18008a510)
0x1800b11b0  mov  r8, rsi             ; context structure
0x1800b11b3  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801703b0` (`0x1801703b0`), which takes `(param_1, param_2)`:

- If `*param_2 == 1`: registers a hook on a cocos2d function, using the string
  `"libcocos2d.dll"` (decoded from immediates `0x636f6362696c`, `0x736f`,
  `0x6c6c642e6432`) and calls `FUN_18010a250()` + `FUN_18008a510()`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

The callback is a standard hook registrar/unregistrar gated on the tag value.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| -- | -- | -- | -- | -- |
| 0x192 | 402 | (registered via FUN_18008f910) | `HACK_NO_PARTICLES` | Master toggle (No Particles) |

## Open questions

- UNVERIFIED: which specific cocos2d/GD method is hooked (the GD method name is not
  recoverable from the registration at `0x1800b11a8` with the available tools; the method
  name pointer is in `rdi = 0x180546628`, an array of function pointers, not a string).
- UNVERIFIED: whether the toggle disables all particles or only specific types.
- UNVERIFIED: which JSON key binds to tag 0x192.
- UNVERIFIED: the vtable/owning class of the callback.

## Method

1. Searched for byte pattern `ba 92 01 00 00` (mov edx, 0x192) in `_original.dll`
   using a Python byte scan. Found 6 occurrences; only one (`0x1800b11a8`) is in the
   Mega Hack code range (below `0x1803A4EE0`).
2. Disassembled `FUN_1800afae0` around `0x1800b11a8` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801703b0` (the callback) with REA/Ghidra 12.1.2 to determine its
   behavior (register/unregister based on `*param_2`).