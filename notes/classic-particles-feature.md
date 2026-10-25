# Classic Particles feature

- **Question** — which code implements the "Classic Particles" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_CLASSIC_PARTICLES` (tag 390, `0x186`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b085c`:

```
0x1800b085c  mov  edx, 0x186          ; tag = HACK_CLASSIC_PARTICLES
0x1800b0861  mov  rcx, rax            ; resolver
0x1800b0864  mov  r8, rsi             ; context structure
0x1800b0867  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180166270` (`0x180166270`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `cocos2d::CCParticleSystem::loadScaledDefaults`
  (mangled: `?loadScaledDefaults@CCParticleSystem@cocos2d@@QEAAXM@Z`), using the
  string `"libcocos2d.dll"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x186 | 390 | (registered via FUN_18008f910) | `HACK_CLASSIC_PARTICLES` | Master toggle (Classic Particles) |

## Method

1. Searched for byte pattern `ba 86 01 00 00` (mov edx, 0x186) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b085c`).
2. Disassembled `FUN_1800afae0` around `0x1800b085c` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180166270` (the callback) with REA/Ghidra 12.1.2 — hooks
   `cocos2d::CCParticleSystem::loadScaledDefaults`.

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x186.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes
  particle scaling defaults).