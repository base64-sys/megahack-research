# No Shake feature

- **Question** — which code implements the "No Shake" cosmetic toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_NO_SHAKE` (tag 409, `0x199`, static-map entry #45 at `0x1800639EA`) is registered as
a hook in the large initializer `FUN_1800afae0` at `0x1800b1560`:

```
0x1800b1552  mov  [rbp + 0x1eb0], rcx     ; rcx = 0x1801718C0 (callback)
0x1800b1560  mov  edx, 0x199              ; tag = HACK_NO_SHAKE
0x1800b1565  mov  rcx, rax                ; resolver
0x1800b1568  mov  r8, rsi                 ; context structure
0x1800b156b  call 0x18008f910             ; hash the tag, bind the callback
```

The callback is materialised immediately before the registration:

```
0x1800b152f  lea  rcx, [rip + 0xc038a]    ; -> 0x1801718C0
0x1800b1536  mov  [rbp + 0xdb0], rcx
0x1800b153d  lea  rdx, [rbp + 0xda8]
0x1800b1544  mov  [rbp + 0xde0], rdx      ; pointer to the {capture, callback} pair
```

So the callback is **`FUN_1801718C0`**, not `FUN_180171620` as the first version of this
note said. `FUN_180171620` belongs to the neighbouring tag `0x185` (No Shaders): a
full-`.text` scan for `lea reg,[rip+disp]` resolving to `0x180171620` finds exactly one
site, `0x1800b14b8`, inside the `0x185` block.

`FUN_1801718C0` takes `(param_1, param_2)` and follows the standard toggle shape:

- `movzx ebx,[rdx]` / `cmp bl,1` / `jne` → unregister path via `FUN_180108a80`.
- `bl == 1` → `call 0x18010a250` (context getter), `mov edx,0x3a5b00` /
  `call 0x18010a370` (resolve GD RVA `0x3A5B00`), then install:

```
0x1801718e8  mov  [rsp + 0x28], 1
0x1801718f0  mov  [rsp + 0x20], 0xffffffe0
0x1801718f8  lea  r8,  [rip + 0x31]        ; -> 0x180171930, detour
0x1801718ff  lea  r9,  [rip + 0x3d3be2]    ; -> 0x1805454E8, "PlayLayer::init"
0x18017190c  call 0x1800ab5b0              ; install
```

`0x1805454E8` reads `"PlayLayer::init"`; it is the only occurrence of that literal.

### The detour

`FUN_180171930` is a scoped-setting wrapper, not a no-op:

1. `call 0x180147ba0` obtains the context object.
2. `lea r12,[rip+0x404486]` at `0x180171952` resolves to `0x180575DDF` = `"0172"`.
   `FUN_180148610(ctx,"0172")` reads the current value into `ebp`;
   `FUN_180149670(ctx,"0172",1)` sets it to 1.
3. `call 0x1801a4f30` forwards the original four register arguments
   (`rcx, rdx, r8d, r9d`).
4. `FUN_180149670(ctx,"0172",ebp)` restores the previous value; the wrapper returns the
   forwarded call's `eax`.

`"0172"` is referenced exactly once in `.text` — at `0x180171952` — so it is a name resolved
at runtime rather than a statically comparable literal. `FUN_180149670` is a TLS-guarded
dispatch (it compares the TLS index and tail-jumps through a function pointer at
`0x180800...`), i.e. a config setter, not a hook helper.

### Corroboration

The Ghidra hook-site pass (`recovered/functions/_hook_sites.tsv`, gitignored) independently
records:

```
0x1801718C0  PlayLayer::init  0x3a5b00  rva  FUN_180171930  function  FUN_1800ab5b0  0xffffffe0
0x180171620  ShaderLayer::performCalculations  0x4880b0  rva  DAT_1801716f0  data  FUN_180171710  0xfffff428
0x180171620  ShaderLayer::visit                0x488a60  rva  DAT_180171700  data  FUN_180171710  0xfffff428
```

`PlayLayer::init` at RVA `0x3A5B00` is a heavily hooked function: `mov edx,0x3a5b00` appears
at 9 sites and 10 functions reference the name string.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x199 | 409 | (resolved via FUN_18008f910) | `HACK_NO_SHAKE` | Master toggle (No Shake) |

## Method

1. Byte-scanned `_original.dll` for `ba 99 01 00 00` (`mov edx,0x199`) and confirmed the
   single Mega Hack-range site `0x1800b1560` with `pe_disasm.py`.
2. Disassembled the whole registration block `0x1800b1523`–`0x1800b1570` to read the
   callable (`lea rcx` at `0x1800b152f`), not the adjacent block.
3. Capstone scan of `.text` for `lea reg,[rip+disp]` pointing at `0x180171620`: one hit,
   in the `0x185` block. Repeated for `0x1801718C0`: one hit, in the `0x199` block.
4. Disassembled `FUN_1801718C0` (`0x1801718c0`, 0x90 bytes) to read the target RVA, the
   friendly-name string and the detour, and resolved the two RIP-relative string operands
   with `scripts/pe_rva.py`.
5. Disassembled the detour `FUN_180171930` and `FUN_180149670` to characterise the
   scoped set/restore behaviour, then scanned `.text` for references to `0x180575DDF` to
   establish that `"0172"` has exactly one use.

## Open questions

- UNVERIFIED: what setting `"0172"` controls, and why it is forced only around
  `PlayLayer::init`. Hypothesis: it is the underlying no-shake flag, and the level-start
  shake is applied during `PlayLayer::init`, so scoping the flag to that call removes the
  start-of-level shake only. Confirming it needs a runtime consumer of `"0172"`.
- UNVERIFIED: which JSON key binds to tag 0x199 (the UI toggle) — this is a different
  binding from `"0172"`.
- Resolved (2026-10-26): "No End Shake" (v9.0.8) is a **separate** feature - tag 439
  (`0x1B7`, no static label) installs a detour on `PlayLayer::showCompleteEffect` (RVA
  `0x3A8F30`) that zeroes `dword [layer+0x520]` after the original runs, whereas `No Shake`
  scopes host setting `"0172"` around `PlayLayer::init`. Note that `COSMETIC/HACK_NO_END_SHAKE`
  exists as a settings key. UNVERIFIED: that `[layer+0x520]` is the shake state rather than the
  completion flash; see [render-hooks.md](render-hooks.md).
