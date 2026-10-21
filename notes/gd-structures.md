# Geometry Dash structures Mega Hack touches

- **Question** — which fields of GD's own structures does Mega Hack read or write,
  and at what byte offsets? (M1 exit criterion: "Geometry Dash structures Mega Hack
  touches documented with verified offsets".)
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-21.
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`.
- **Tools** — `pe_disasm.py` (capstone), manual recovered `.c` reads, `_rtti.tsv`,
  `_host_methods.tsv`, `_calls.tsv`, `pe_rva.py`, and a machine-code sweep of every
  direct memory operand in `.text` (displacement scan, `.pdata`-derived function
  ownership).

## Method

Mega Hack imports zero GD gameplay symbols (see `notes/host-abi.md`), so every
GJGameLevel access is a hard-coded byte displacement on a pointer obtained at
runtime (via `FUN_1801ACDB0` — current level, 50 callers — or via a field such as
`param + 0x878` / `LevelPage + 0x1A8`). A displacement alone is weak evidence, so
each offset below is confirmed from two independent angles:

1. a **disassembly view** showing the literal displacement in a `mov`/`lea`/`movups`;
2. a **structural tie** to GJGameLevel — either the object's vtable is set to
   `GJGameLevel::vftable` (`0x180572C48`, `_rtti.tsv` row 240), or the surrounding
   field cluster (level ID at `+0x148`/`+0x14C`, name string at `+0x158`) matches
   the GJGameLevel layout recovered from `FUN_180145430`.

Function ranges and VA→function mapping come from `recovered/functions/_index.tsv`.
RVAs were resolved with `scripts/pe_rva.py` semantics (file-backed only); all
addresses below are preferred-base VAs in `absolllute.hackmega_original.dll`.

## Verified offsets on `GJGameLevel`

| Offset | Access kind | Referencing function (VA) | Instruction VA | Evidence (angle 2) |
| ------ | ----------- | ------------------------ | -------------- | ------------------ |
| `+0x148` | read (int) | `FUN_18020BD00` `0x18020BD00` | `0x18020BDD3` `mov ecx, dword ptr [rax + 0x148]` | second angle: `FUN_180153CD0` `0x180153CD0` reads `[rax+0x148]` on the object at `LevelPage+0x1A8` (`0x180153D73`); same level-ID cluster. |
| `+0x14C` | read (int) | `FUN_18020BD00` `0x18020BD00` | `0x18020BDD9` `sub ecx, dword ptr [rax + 0x14C]` | second angle: `FUN_180153CD0` `0x180153CD0` `0x180153D79` `[rax+0x14C]`. |
| `+0x150` | write (int) | `FUN_18020BD00` `0x18020BD00` | `0x18020BDDF` `mov dword ptr [rax + 0x150], ecx` | derived as `+0x148 - +0x14C` (level ID); same object. (Not one of the seven, recorded for completeness.) |
| `+0x158` | read (std::string) / init | `FUN_18020BD00` `0x18020BD00` | `0x18020BDF2` `lea r9, [rax + 0x158]`; `0x18020BE16` `mov r9, qword ptr [rax + 0x158]` | second angle: `FUN_180145430` `0x180145430` sets the object's vtable to `GJGameLevel::vftable` (`0x180572C48`) and zeroes the field with `movups xmmword ptr [rsi + 0x158], xmm0` (`0x180145464`). |
| `+0x198` | init (std::string, 16 bytes) | `FUN_180145430` `0x180145430` | `0x18014549E` `movups xmmword ptr [rsi + 0x198], xmm0` | second angle: same function sets `GJGameLevel::vftable` at `[rsi]` (`lea rax, [rip+0x42D7EA]` → `0x180572C48`), so the object is a GJGameLevel; `+0x198` sits at the same std::string slot position as `+0x158`. |
| `+0x1B8` | init (std::string, 16 bytes) | `FUN_180145430` `0x180145430` | `0x1801454BB` `movups xmmword ptr [rsi + 0x1B8], xmm0` | second angle: same as `+0x198` (vtable = GJGameLevel). |

## Unverified offsets

Both displacements were swept at **machine-code** level, not by grepping the
decompiled tree: every direct memory operand in `.text` whose displacement equals
the offset was enumerated (capstone 5.0.9 over `.text`, file mapping through
`scripts/pe_rva.py`, function ownership from the `.pdata` runtime-function table).
That is the only way to answer "did we miss an access?", because the decompiler
folds pointer arithmetic away.

### `+0x2C4` — 18 instruction sites in 14 functions, none on a GJGameLevel

| Function (VA) | Site (VA) | Instruction | Width |
| ------------- | --------- | ----------- | ----- |
| `FUN_1800F0B80` `0x1800F0B80` | `0x1800F0B9B` | `mov dword ptr [rcx + 0x2c4], edx` | 4 |
| `FUN_1800F0CC0` `0x1800F0CC0` | `0x1800F0D0A` | `movzx ecx, byte ptr [rdi + 0x2c4]` | **1** |
| `FUN_1800F0E60` `0x1800F0E60` | `0x1800F0EA3` | `cvtsi2ss xmm0, dword ptr [rdi + 0x2c4]` | 4 |
| *(no `.pdata` entry)* | `0x1800F1B00` | `mov eax, dword ptr [rcx + 0x2c4]` | 4 |
| `FUN_1800F1B10` `0x1800F1B10` | `0x1800F1B55` | `cmp edx, dword ptr [rcx + 0x2c4]` | 4 |
| `FUN_1800F1B70` `0x1800F1B70` | `0x1800F1B87` | `mov eax, dword ptr [rcx + 0x2c4]` | 4 |
| `FUN_1800F1C00` `0x1800F1C00` | `0x1800F1C0B` | `add edx, dword ptr [rcx + 0x2c4]` | 4 |
| `FUN_1800F1D40` `0x1800F1D40` | `0x1800F1D51` | `mov dword ptr [rsi + 0x2c4], eax` | 4 |
| `FUN_18010D1A0` `0x18010D1A0` | `0x18010D24D` | `mov r9d, dword ptr [rax + 0x2c4]` | 4 |
| `FUN_1801312B0` `0x1801312B0` | `0x18013133E` | `mov dword ptr [rax + 0x2c4], esi` | 4 |
| `FUN_180134B70` `0x180134B70` | `0x180134C40` | `mov eax, dword ptr [rcx + 0x2c4]` | 4 |
| `FUN_18017BF70` `0x18017BF70` | `0x18017BFDD`, `0x18017BFE5` | `movsd xmm0/qword ptr [rdi + 0x2c4]` / `movsd qword ptr [rsi + 0x2c4], xmm0` | **8** |
| `FUN_18017C7C0` `0x18017C7C0` | `0x18017C85B`, `0x18017C863` | `movss xmm0/dword ptr [rdi + 0x2c4]` / `movss dword ptr [rsi + 0x2c4], xmm0` | 4 (float) |
| `FUN_1802063C0` `0x1802063C0` | `0x1802068E7` | `movsd qword ptr [r12 + 0x2c4], xmm0` | **8** |
| `FUN_180207470` `0x180207470` | `0x180207BD5` | `movsd xmm0, qword ptr [rdx + 0x2c4]` | **8** |
| `FUN_18024AEF0` `0x18024AEF0` | `0x18024AF1B` | `mov dword ptr [rsi + 0x2c4], 0` | 4 |

The same displacement is read as a **byte** (`0x1800F0D0A`), as a **dword**
(most sites), as a **float** (`0x18017C85B`) and as a **qword** (`0x18017BFDD`,
`0x1802068E7`, `0x180207BD5`). One field cannot be four different types in one
structure: these are **different structures that share a displacement**. That
alone disqualifies `+0x2C4` as a single named field, let alone a GJGameLevel one.

Two of the sites touch a structure that is not Mega Hack's own:

- `FUN_1801312B0` `0x18013133E` — writes the **GD host object** returned by
  `FUN_180147BA0`. See the next section; this is the one `+0x2C4` site that is
  *not* a Mega Hack-internal structure.
- `FUN_18010D1A0` `0x18010D24D` — reads a dword from `*(param_1 + 8)`, a
  `cocos2d::CCObject` (the same function ends with
  `cocos2d::CCObject::release(*(CCObject **)(param_1 + 8))`,
  `recovered/functions/18010d1a0_FUN_18010d1a0.c:35`). That object is **not** a
  GJGameLevel: its `+0x1B8` is read as a **dword** (`0x18010D2E7`), whereas a
  GJGameLevel's `+0x1B8` is a `std::string` (initialised by `FUN_180145430`).

The remaining sites are Mega Hack's own structures: `FUN_1802063C0` /
`FUN_180207470` treat `+0x148` as a **byte** (`0x1802065F4`, `0x180207693`) and
`+0x198` as a **float** (`0x180207793`), which is incompatible with the
GJGameLevel layout, so that pair is a third structure. `FUN_18017BF70` /
`FUN_18017C7C0` copy a qword / float to and from the same displacement, i.e. a
plain 8- or 4-byte field in a Mega Hack-side record.

The GJGameLevel constructor `FUN_180145430` does not touch `+0x2C4`.

**UNVERIFIED on a GJGameLevel.** What would confirm it: a GD-side reference
(`GeometryDash.exe` is not in this repo) or a runtime observation of a
GJGameLevel field at this displacement.

### `+0x35C` — 5 instruction sites in 3 functions, all on the GD host object

| Function (VA) | Site (VA) | Instruction |
| ------------- | --------- | ----------- |
| `FUN_18010BBD0` `0x18010BBD0` | `0x18010BC1A`, `0x18010BC67`, `0x18010BC96` | `mov dword ptr [rax/rsi + 0x35c], ebp/eax` |
| `FUN_180131C70` `0x180131C70` | `0x180131CF9` | `mov dword ptr [rax + 0x35c], esi` |
| `FUN_180135160` `0x180135160` | `0x18013524D` | `mov dword ptr [rcx + 0x35c], eax` |

All three functions take the pointer from the same source:

```
FUN_18010BBD0:  lVar2 = FUN_180147ba0();  ... *(int *)(lVar2 + 0x35c) = iVar4;
FUN_180131C70:  lVar1 = FUN_180147ba0();  ... *(int *)(lVar1 + 0x35c) = param_4;
FUN_180135160:  lVar3 = FUN_180147ba0();  ... *(int *)(lVar3 + 0x35c) = iVar2;
```

(`recovered/functions/18010bbd0_FUN_18010bbd0.c`, `180131c70_FUN_180131c70.c`,
`180135160_FUN_180135160.c`.)

`FUN_180147BA0` (`0x180147BA0`) is a lazy resolver, not an object accessor:

```
0x180147bf4  call   0x18010ddc0                ; GetModuleHandleA(NULL) -> GD base
0x180147bf9  lea    rcx, [rax + 0x17b4a0]       ; host RVA 0x17B4A0
0x180147c00  call   0x180147c60                 ; resolve
0x180147c05  mov    qword ptr [rip + 0x6c21a4], rax   ; -> 0x180809DB0
0x180147c1e  jmp    qword ptr [rip + 0x6c218b]        ; -> 0x180809DB0 (tail call)
```

So the pointer MH manipulates is the **return value of a function inside
`GeometryDash.exe` at host RVA `0x17B4A0`**. What that function is, and what
class it returns, is not established here: `GeometryDash.exe` is not in this repo.
Two structural facts about the returned object are established, though:

- it holds a pointer at `+0x208` that MH reads as the **current level**
  (`FUN_1801ACDB0` `0x1801ACDB9`, `mov rax, qword ptr [rax + 0x208]`, reached
  through the 5-byte thunk `FUN_18014E690` → `jmp 0x180147BA0`);
- it carries its own run of `int` triples — `(0x348,0x34C)→0x350`,
  `(0x354,0x358)→0x35C`, `(0x360,0x364)→0x368`, written by `FUN_180131C70` and
  `FUN_180135160` — plus nine more at `0x2C4`–`0x320` and `0x36C`–`0x374`
  written by `FUN_1801312B0` (`recovered/functions/1801312b0_FUN_1801312b0.c`,
  cases 0–8).

That triple layout is **not** GJGameLevel's. A GJGameLevel's identical-shaped
triple is at `+0x148`/`+0x14C`→`+0x150` (see the verified table above), and the
GJGameLevel constructor `FUN_180145430` initialises `std::string`s at `+0x158`,
`+0x198` and `+0x1B8` — no such access is made on this object at those
offsets.

**UNVERIFIED on a GJGameLevel.** What would confirm it: a GD-side reference or a
runtime observation.

### The two offsets share one object, and the release never compares them

This corrects an earlier statement in this note, which said no function accesses
both `+0x2C4` and `+0x35C` on the same object. That is wrong: **both offsets
live on the same GD host object**, reached through the same resolver
`FUN_180147BA0`.

- `FUN_1801312B0` (`0x18013133E`) writes `+0x2C4` on the object returned by
  `FUN_180147BA0` (`recovered/functions/1801312b0_FUN_1801312b0.c:25-29`:
  `lVar1 = FUN_180147ba0(); … *(int *)(lVar1 + 0x2c4) = param_4 + *(int *)(lVar1 + 0x2c8);`).
- `FUN_18010BBD0`, `FUN_180131C70` and `FUN_180135160` write `+0x35C` on that same
  object.

What is true is the weaker statement: **no single function touches both**, and no
release instruction compares `[+0x2C4]` with `[+0x35C]`. Across all 18 `+0x2C4`
sites the only compare is `cmp edx, dword ptr [rcx + 0x2c4]` (`0x1800F1B55`),
against a register; across the 5 `+0x35C` sites there is no compare at all. So
the release contains **no mirror check** between the two fields, even though both
are fields of one object.

## Notes on the level-ID field cluster

`FUN_18020BD00` shows the GJGameLevel identity computation Mega Hack relies on:

```
0x18020BDD3  mov  ecx, dword ptr [rax + 0x148]   ; level id high
0x18020BDD9  sub  ecx, dword ptr [rax + 0x14C]   ; minus level id low
0x18020BDDF  mov  dword ptr [rax + 0x150], ecx   ; stored level id
0x18020BDF2  lea  r9, [rax + 0x158]             ; level name (std::string)
0x18020BE16  mov  r9, qword ptr [rax + 0x158]    ; name data ptr (SSO-aware)
```

where `rax = *(param_2 + 0x878)` (a GJGameLevel*). `FUN_180153CD0` performs the
same `+0x148 - +0x14C` on the object at `LevelPage + 0x1A8`, confirming the
field cluster is GD's, not Mega Hack's invention.

## Related verified offset (already in notes/hook-dispatch.md)

- `PlayLayer + 0x3230` — one-byte re-entrancy flag set around the original
  `levelComplete`. Verified from `FUN_180199E30` (`0x180199E30`).

## Conclusion

Five of the documented GJGameLevel offsets (`+0x148`, `+0x14C`, `+0x158`,
`+0x198`, `+0x1B8`) are confirmed in `_original.dll` from two independent
angles each. `+0x2C4` and `+0x35C` are **not** accessed on a GJGameLevel in
this binary: they are fields of a different GD structure (the object returned by
the host function at RVA `0x17B4A0`), and they remain UNVERIFIED as GJGameLevel
offsets. The M1 criterion therefore remains open; only the five above should be
quoted as verified.