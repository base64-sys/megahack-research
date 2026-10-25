# No Robot/Swing Fire feature

- **Question** — which code implements the "No Robot/Swing Fire" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

Two separate tags control two separate hooks:

### No Robot Fire
`HACK_NO_ROBOT_FIRE` (tag 440, `0x1B8`) is registered as a hook in `FUN_1800afae0`
at `0x1800b1472`:

```
0x1800b1472  mov  edx, 0x1B8          ; tag = 440 (No Robot Fire)
0x1800b1477  mov  rcx, rax            ; resolver
0x1800b147a  mov  r8, rsi             ; context structure
0x1800b147d  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180171550` (`0x180171550`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayerObject::toggleRobotMode`
  (RVA `0x39B6F0`) via `FUN_1800AAFE0`, using the string
  `"PlayerObject::toggleRobotMode"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108A80()`.

The callback `FUN_1801715c0` (`0x1801715c0`) sets a value to 0 (disabling the fire effect).

### No Swing Fire
`HACK_NO_SWING_FIRE` (tag 441, `0x1B9`) is registered as a hook in `FUN_1800afae0`
at `0x1800b164e`:

```
0x1800b164e  mov  edx, 0x1B9          ; tag = 441 (No Swing Fire)
0x1800b1653  mov  rcx, rax            ; resolver
0x1800b1656  mov  r8, rsi             ; context structure
0x1800b1659  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180171ca0` (`0x180171ca0`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayerObject::toggleSwingMode`
  (RVA `0x39AB20`) via `FUN_1800AAFE0`, using the string
  `"PlayerObject::toggleSwingMode"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108A80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1B8 | 440 | (registered via FUN_18008f910) | (not in static list — runtime-assigned) | Master toggle (No Robot Fire) |
| 0x1B9 | 441 | (registered via FUN_18008f910) | (not in static list — runtime-assigned) | Master toggle (No Swing Fire) |

## Method

1. Searched for string `"PlayerObject::toggleRobotMode"` in `_original.dll` using REA
   `search_strings` — found at `0x180545413`.
2. Found xrefs to the string: `0x1800a879b` (in `FUN_1800a83b0`, initial hook setup)
   and `0x18017158f` (in `FUN_180171550`, the tag-gated registrar).
3. Found xrefs to `FUN_180171550`: `0x1800b1441`, `0x1800b1448`, `0x1800b1464`
   (all in `FUN_1800afae0`). Disassembled at `0x1800b1438` with `pe_disasm.py`
   to confirm tag `0x1B8` (440) and the call to `FUN_18008f910`.
4. Repeated the process for `toggleSwingMode`: found tag `0x1B9` (441) at
   `0x1800b164e` in `FUN_1800afae0`.

## Open questions

- UNVERIFIED: which JSON keys bind to tags 0x1B8 and 0x1B9.
- UNVERIFIED: whether "No Robot Fire" and "No Swing Fire" are separate toggles or
  a single combined feature ("No Robot/Swing Fire" in the changelog).
- UNVERIFIED: the exact behavior of the hooks (whether they veto or observe).