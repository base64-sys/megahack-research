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

### The detour body (2026-10-28)

`FUN_1801734c0` (`0x1801734C0..0x180173593`, 211 bytes, `.pdata` via `scripts/func_bounds.py`) is
the `CCMotionStreak::initWithFade` detour. It is a **value transformer**, not a veto: it scales
the fade duration and then calls the original.

| Step | Evidence |
| --- | --- |
| registry = `FUN_18008A510()` | first call in the body |
| `rate = getConfigVar<double>(393)` | `0x1801734FA mov edx,0x189` then `call 0x180003D80` |
| bound | `ucomisd` against the double `10.0` at `.rdata` `0x1805756A0`, `jbe` to the bounded path; the constants `0.01` (`.rdata` `0x180575F40`) and `0.1` (`.rdata` `0x180575F48`) are the two branches |
| scale | the fade argument is multiplied by the double `0.0784313753247261` at `.rdata` `0x180575F50` (`mulsd` at `0x18017353E`) before the original is called |
| post-store | `movss` from `.rdata` `0x180534350` (float `0.0`) into `[streak+0x17C]` at `0x180173573`; the decompiler renders the success path as `[streak+0x17C] = 1.0 / fade` (UNVERIFIED operand order) |

Tag 393 (`0x189`) is the **Trail Cutting Rate**: a whole-`.text` scan for `mov edx,0x189`
returns exactly one site, `0x1801734FA`, so the rate is read only here. It has no static
`HACK_*` label (the 85-entry map in [static-hack-label-map.md](static-hack-label-map.md) has no
entry with value 393), which is why the earlier label join proposed only tag 392.

The `0.0784313753247261` factor is a data constant at `.rdata` `0x180575F50`, applied
unconditionally (`mulsd` at `0x18017353E`); its origin is not established. The decompiler's
`m = 1.0 / max(rate, 0.01)` reading is consistent with the two bound constants but the exact
operand order in the stream is not transcribed here (`0.25` at `.rdata` `0x180534358` is loaded
at `0x180173527`), so the arithmetic is stated at decompiler level only.

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x188 and which binds to tag 0x189. Neither appears
  in the settings samples; the row labels exist only in the mod's own settings tree.
- UNVERIFIED: the unit of the 393 rate and the meaning of the `0.078431375` factor, and whether
  the `rate > 10` branch (`m = 0.1`) is a deliberate ceiling or an error path.
- UNVERIFIED: no runtime observation of a cut trail; the claim is static, from the detour body.
