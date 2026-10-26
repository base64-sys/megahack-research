# Auto Deafen feature

- **Question** — which code implements the "Auto Deafen" audio toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_AUTO_DEAFEN` (tag 279, `0x117`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b1dc8`:

```
0x1800b1dc8  mov  edx, 0x117          ; tag = HACK_AUTO_DEAFEN
0x1800b1dcd  mov  rcx, rax            ; resolver
0x1800b1dd0  mov  r8, rsi             ; context structure
0x1800b1dd3  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180174a80` (`0x180174a80`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayLayer::resetLevel`
  (RVA `0x3B8EB0`) via `FUN_1800AB850`, using the string
  `"PlayLayer::resetLevel"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x117 | 279 | (registered via FUN_18008f910) | `HACK_AUTO_DEAFEN` | Master toggle (Auto Deafen) |

## Method

1. Searched for byte pattern `ba 17 01 00 00` (mov edx, 0x117) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b1dc8`).
2. Disassembled `FUN_1800afae0` around `0x1800b1dc8` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180174a80` (the callback) with REA/Ghidra 12.1.2 — hooks
   `PlayLayer::resetLevel` (RVA `0x3B8EB0`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x117.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes level resets).
- UNVERIFIED: how PlayLayer::resetLevel relates to audio deafening.


Correction (2026-10-26): a leaked tool-call transcript tail had been appended to this
file and was not research content. It has been removed; nothing above it was changed.
