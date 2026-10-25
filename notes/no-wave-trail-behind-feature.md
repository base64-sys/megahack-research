# No Wave Trail Behind feature

- **Question** — which code implements the "No Wave Trail Behind" gameplay toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_WAVE_TRAIL_BEHIND` (tag 414, `0x19E`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b182a`:

```
0x1800b182a  mov  edx, 0x19E          ; tag = HACK_NO_WAVE_TRAIL_BEHIND
0x1800b182f  mov  rcx, rax            ; resolver
0x1800b1832  mov  r8, rsi             ; context structure
0x1800b1835  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180172280` (`0x180172280`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayerObject::activateStreak`
  (RVA `0x3A0090`) via `FUN_1800AB230`, using the string
  `"PlayerObject::activateStreak"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x19E | 414 | (registered via FUN_18008f910) | `HACK_NO_WAVE_TRAIL_BEHIND` | Master toggle (No Wave Trail Behind) |

## Method

1. Searched for byte pattern `ba 9e 01 00 00` (mov edx, 0x19E) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b182a`).
2. Disassembled `FUN_1800afae0` around `0x1800b182a` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180172280` (the callback) with REA/Ghidra 12.1.2 — hooks
   `PlayerObject::activateStreak` (RVA `0x3A0090`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x19E.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes streak activation).