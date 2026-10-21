# Geometry Dash structures Mega Hack touches

- **Question** — which fields of GD's own structures does Mega Hack read or write,
  and at what byte offsets? (M1 exit criterion: "Geometry Dash structures Mega Hack
  touches documented with verified offsets".)
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-21.
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`.
- **Tools** — `pe_disasm.py` (capstone), manual recovered `.c` reads, `_rtti.tsv`,
  `_host_methods.tsv`, `_calls.tsv`, `pe_rva.py`.

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

- **`+0x2C4`** — UNVERIFIED on a GJGameLevel in this binary. No function in
  `_original.dll` reads or writes `+0x2C4` on an object whose field cluster matches
  GJGameLevel. `FUN_180207470` (`0x180207470`) does access `[rdx + 0x2C4]`
  (`0x180207BD5`, `movsd xmm0, qword ptr [rdx + 0x2C4]`), but the same function
  treats `+0x198`/`+0x1B8`/`+0x158` as scalar floats/doubles, not as the
  std::string fields of the GJGameLevel layout, so that object is a different
  structure (layout coincidence, not identity). The constructor `FUN_180145430` does
  not touch `+0x2C4`. What would confirm it: a GD-side reference (`GeometryDash.exe`)
  or a runtime observation of a GJGameLevel's editor ID at this displacement.
- **`+0x35C`** — UNVERIFIED on a GJGameLevel. The only `+0x35C` accesses in the
  binary (`FUN_18010BBD0` `0x18010BBD0`, `FUN_180131C70` `0x180131C70`,
  `FUN_180135160` `0x180135160`) are on the object returned by `FUN_180147BA0`
  (the GD host accessor at RVA `0x17B4A0`), not on a GJGameLevel, and store a
  computed level-ID difference there. No function accesses both `+0x2C4` and `+0x35C`
  on the same object, so the patch's mirror check `[+0x2C4] == [+0x35C]` is not
  present in the release. What would confirm it: a GD-side reference or runtime
  observation.

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

Five of the seven documented GJGameLevel offsets (`+0x148`, `+0x14C`, `+0x158`,
`+0x198`, `+0x1B8`) are confirmed in `_original.dll` from two independent
angles each. `+0x2C4` and `+0x35C` are not accessed on a GJGameLevel in this
binary and are marked UNVERIFIED. The M1 criterion therefore remains open; only
the five above should be quoted as verified.