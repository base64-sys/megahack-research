# No Shaders feature

- **Question** — which code implements the "No Shaders" render toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_SHADERS` (tag 389, `0x185`) is registered as a hook in the large initializer
`FUN_1800afae0` (`0x1800afae0`) at `0x1800b14e9`:

```
0x1800b14e9  mov  edx, 0x185          ; tag = HACK_NO_SHADERS
0x1800b14ee  mov  rcx, rax            ; resolver
0x1800b14f1  mov  r8, rsi             ; context structure
0x1800b14f4  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180171620` (`0x180171620`), materialised immediately before the
registration:

```
0x1800b14b8  lea  rcx, [rip + 0xc0161]    ; -> 0x180171620
0x1800b14e9  mov  edx, 0x185              ; tag = HACK_NO_SHADERS
0x1800b14f4  call 0x18008f910
```

It takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers hooks on `ShaderLayer::performCalculations`
  (RVA `0x4880B0`, name string `0x180575DAB`) and `ShaderLayer::visit` (RVA `0x488A60`,
  name string `0x180575DCC`) via `FUN_180171710`, order constant `0xfffff428`.
- If `*param_2 != 1`: unregisters both hooks via `FUN_180108a80()`.

**Correction (2026-10-25):** this note previously claimed the callback was shared with
`HACK_NO_SHAKE` (tag 409, `0x199`), and repeated that claim as a finding. It is not.
No Shake's registration block materialises a different callable,
`0x1800b152f  lea rcx,[rip+0xc038a]` → `FUN_1801718C0`, which hooks `PlayLayer::init`
(RVA `0x3A5B00`). A full-`.text` scan for `lea reg,[rip+disp]` resolving to `0x180171620`
finds exactly one site, `0x1800b14b8`, inside this `0x185` block. See
[no-shake-feature.md](no-shake-feature.md).

`HACK_NO_SHADERS` is not one of the 389 `Added …` changelog rows: it is named under
`New For 2.2 Mods`, which the feature inventory does not yet cover.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x185 | 389 | (registered via FUN_18008f910) | `HACK_NO_SHADERS` | Master toggle (No Shaders) |

## Method

1. Searched for byte pattern `ba 85 01 00 00` (mov edx, 0x185) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b14e9`).
2. Disassembled `FUN_1800afae0` around `0x1800b14e9` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180171620` (the callback) with REA/Ghidra 12.1.2 — hooks
   `ShaderLayer::performCalculations` and `ShaderLayer::visit`.

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x185.
- UNVERIFIED: what the two `ShaderLayer` detours do. The Ghidra hook-site pass records
  the detours as `data` labels (`DAT_1801716f0`, `DAT_180171700`), so their bodies have not
  been read yet; whether they veto the call or observe it is open.
- UNVERIFIED: whether `HACK_NO_SHADERS` and `HACK_NO_SHAKE` interact in practice. They are
  independent hooks (`ShaderLayer` vs `PlayLayer::init`) with separate callbacks, but both
  ultimately affect what is drawn, so a combined-state test is still worth doing.