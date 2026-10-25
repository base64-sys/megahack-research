# Trail Cutting feature

- **Question** — which code implements the "Trail Cutting" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_TRAIL_CUTTING` (tag 392, `0x188`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b1be2`:

```
0x1800b1be2  mov  edx, 0x188          ; tag = HACK_TRAIL_CUTTING
0x1800b1be7  mov  rcx, rax            ; resolver
0x1800b1bea  mov  r8, rsi             ; context structure
0x1800b1bed  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_1801732c0` (`0x1801732c0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `cocos2d::CCMotionStreak::initWithFade`
  (mangled: `?initWithFade@CCMotionStreak@cocos2d@@QEAA_N...`) via `FUN_1803a50b4`,
  using the string `"libcocos2d.dll"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x188 | 392 | (registered via FUN_18008f910) | `HACK_TRAIL_CUTTING` | Master toggle (Trail Cutting) |

## Method

1. Searched for byte pattern `ba 88 01 00 00` (mov edx, 0x188) in `_original.dll`
   using a Python byte scan. Found 3 occurrences; only one (`0x1800b1be2`) in the
   Mega Hack code range.
2. Disassembled `FUN_1800afae0` around `0x1800b1be2` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_1801732c0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `cocos2d::CCMotionStreak::initWithFade`.

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x188.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes trail cutting).
- UNVERIFIED: how the hook modifies trail cutting (the callback `FUN_1801734c0`
  is not yet analyzed).