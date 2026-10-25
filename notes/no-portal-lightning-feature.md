# No Portal Lightning feature

- **Question** — which code implements the "No Portal Lightning" cosmetic toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_PORTAL_LIGHTNING` (tag 416, `0x1A0`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b1296`:

```
0x1800b1296  mov  edx, 0x1A0          ; tag = HACK_NO_PORTAL_LIGHTNING
0x1800b129b  mov  rcx, rax            ; resolver
0x1800b129e  mov  r8, rsi             ; context structure
0x1800b12a1  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180171170` (`0x180171170`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `GJBaseGameLayer::lightningFlash`
  (RVA `0x246D00`) via `FUN_1801711F0`, using the string
  `"GJBaseGameLayer::lightningFlash"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1A0 | 416 | (registered via FUN_18008f910) | `HACK_NO_PORTAL_LIGHTNING` | Master toggle (No Portal Lightning) |

## Method

1. Searched for contiguous string `"HACK_NO_PORTAL_LIGHTNING"` in `_original.dll`
   using REA `search_strings` — found at `0x18053F3DC`.
2. Byte-scanned for `ba a0 01 00 00` (mov edx, 0x1A0) — found 7 occurrences,
   only one (`0x1800b1296`) in the Mega Hack code range.
3. Disassembled `FUN_1800afae0` around `0x1800b1296` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
4. Analyzed `FUN_180171170` (the callback) with REA/Ghidra 12.1.2 — hooks
   `GJBaseGameLayer::lightningFlash` (RVA `0x246D00`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x1A0.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes).