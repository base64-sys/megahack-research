# No (C) Mark feature

- **Question** — which code implements the "No (C) Mark" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_C_MARK` (tag 44, `0x02C`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b049a`:

```
0x1800b049a  mov  edx, 0x2C           ; tag = HACK_NO_C_MARK
0x1800b049f  mov  rcx, rax            ; resolver
0x1800b04a2  mov  r8, rsi             ; context structure
0x1800b04a5  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180162d20` (`0x180162d20`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `ShareLevelLayer::onShare`
  (RVA `0x490270`) via `FUN_180162E10`, using the string
  `"ShareLevelLayer::onShare"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x02C | 44 | (registered via FUN_18008f910) | `HACK_NO_C_MARK` | Master toggle (No (C) Mark) |

## Method

1. Searched for byte pattern `ba 2c 00 00 00` (mov edx, 0x2C) in `_original.dll`
   using a Python byte scan. Found 2 occurrences; only one (`0x1800b049a`) in the
   Mega Hack code range.
2. Disassembled `FUN_1800afae0` around `0x1800b049a` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180162d20` (the callback) with REA/Ghidra 12.1.2 — hooks
   `ShareLevelLayer::onShare` (RVA `0x490270`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x02C.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes share events).
- UNVERIFIED: how ShareLevelLayer::onShare relates to the (C) mark on levels.