# No Death Effect feature

- **Question** — which code implements the "No Death Effect" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_DEATH_EFFECT` (tag 399, `0x18F`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b0df0`:

```
0x1800b0df0  mov  edx, 0x18F          ; tag = HACK_NO_DEATH_EFFECT
0x1800b0df5  mov  rcx, rax            ; resolver
0x1800b0df8  mov  r8, rsi             ; context structure
0x1800b0dfb  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180169330` (`0x180169330`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: resolves the target as the same-image RVA `0x37EF10`
  (`mov edx, 0x37ef10` / `call 0x18010a370`), then calls `FUN_1800ab230` with the detour
  `0x180169400` (`lea r8,[rip+0x8d]` at `0x18016936c`) and the friendly name
  `"PlayerObject::playDeathEffect"` (string at `0x180545488`, `lea r9,[rip+0x3dc10e]` at
  `0x180169373`).
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

The detour at `0x180169400` is the entire function `c3` — a bare `ret`. The toggle
therefore works by replacing `PlayerObject::playDeathEffect` with a no-op, not by
observing and then vetoing.

The same callback installs a **second** hook, found on 2026-10-25 with
`scripts/callback_hooks.py` and not previously recorded:

| Target | RVA | Detour | Installer |
| --- | --- | --- | --- |
| `PlayLayer::destroyPlayer` | `0x3B39D0` | `0x180169410` | `FUN_180169480` |

Both hooks are installed on the same `*param_2 == 1` branch, and both are removed through
`FUN_180108a80` on the other branch. UNVERIFIED: what the `destroyPlayer` detour does and
whether it is the mechanism behind the changelog's "No Death Effect in duals" fix
([feature-inventory.md](feature-inventory.md)); the detour body was not read this pass.

Verified 2026-10-25 with `pe_disasm.py` on `FUN_180169330` and `0x180169400`, and
`scripts/pe_rva.py` for the name string. Note: an intermediate arithmetic slip put the
name string at `0x180575488` (which holds a pair of pointers) instead of
`0x180545488`; the raw string scan for `PlayerObject::playDeathEffect` found the single
occurrence at `0x180545488`, which is the check that caught it.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x18F | 399 | (registered via FUN_18008f910) | `HACK_NO_DEATH_EFFECT` | Master toggle (No Death Effect) |

## Method

1. Searched for byte pattern `ba 8f 01 00 00` (mov edx, 0x18F) in `_original.dll`
   using a Python byte scan. Found 2 occurrences; only one (`0x1800b0df0`) in the
   Mega Hack code range.
2. Disassembled `FUN_1800afae0` around `0x1800b0df0` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Disassembled `FUN_180169330` (the callback) with `pe_disasm.py` — resolves RVA
   `0x37EF10` and installs the bare-`ret` detour at `0x180169400`.
4. Cross-checked the friendly name by scanning the image for the literal
   `PlayerObject::playDeathEffect`.

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x18F.
- UNVERIFIED: whether GD's `playDeathEffect` has a return value the `ret` detour leaves
  undefined, and whether every caller ignores it (a void `ret` is only safe if the
  function's result is unused).
- UNVERIFIED: how the "No Death Effect in duals" fix (changelog line 893) is expressed;
  the single hook here would already cover both players if both use the same method.