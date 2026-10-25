# No Trail feature

- **Question** — which code implements the "No Trail" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_TRAIL` (tag 395, `0x18B`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b16c5`:

```
0x1800b16c5  mov  edx, 0x18B          ; tag = HACK_NO_TRAIL
0x1800b16ca  mov  rcx, rax            ; resolver
0x1800b16cd  mov  r8, rsi             ; context structure
0x1800b16d0  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180171db0` (`0x180171db0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `cocos2d::CCMotionStreak::draw`
  (mangled: `?draw@CCMotionStreak@cocos2d@@UEAAXXZ`), using the string
  `"libcocos2d.dll"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x18B | 395 | (registered via FUN_18008f910) | `HACK_NO_TRAIL` | Master toggle (No Trail) |

## Method

1. Searched for byte pattern `ba 8b 01 00 00` (mov edx, 0x18B) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b16c5`).
2. Disassembled `FUN_1800afae0` around `0x1800b16c5` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180171db0` (the callback) with REA/Ghidra 12.1.2 — hooks
   `cocos2d::CCMotionStreak::draw` (mangled `?draw@CCMotionStreak@cocos2d@@UEAAXXZ`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x18B.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes trail rendering).