# CRT static initializers, and why "zero callers" does not mean dead code

## Question

`FUN_180062940` wires up 77 UI element names, yet Ghidra attributed **no callers** to it.
Dead code that survived linking, or live code? Nothing in the call graph can tell the two
apart, because a call graph built from direct `call` instructions is blind to every function
reached through a pointer. This note recovers the pointer-reached set.

Re-derived independently with `scripts/init_table.py`, which finds the tables from the
`_initterm` import rather than from a hard-coded address.

## Method

1. Parse the import directory with `pe_rva.py` primitives only, and resolve the IAT slots for
   `_initterm` and `_initterm_e`.
2. Scan executable sections for `FF 25 <rel32>` (`jmp qword ptr [rip+disp32]`) whose computed
   target equals one of those slots — the IAT jump thunks.
3. Scan for `E8 <rel32>` (`call rel32`) whose destination is a thunk — the call sites.
4. At each call site, walk back for the nearest `48 8D 05` / `48 8D 15` pair, which is
   `lea rcx, [rip+..]` / `lea rdx, [rip+..]` — the `(first, last)` range.

No capstone and no pefile, so the tool runs on a fresh clone with no sample set present.
Only those four instruction prefixes are decoded, and each is filtered by a strong predicate
(the IAT slot, the thunk address, an 8-byte-aligned `.rdata` target), so the linear scan does
not accumulate false positives.

## Result

| Symbol | Call site | Thunk | Range | Slots | Non-zero |
| ------ | --------- | ----- | ----- | ----: | -------: |
| `_initterm` | `0x180531221` | `0x180533E10` | `[0x18063F068, 0x18063F938)` | 282 | **281** |
| `_initterm_e` | `0x180531201` | `0x180533E20` | `[0x18063F940, 0x18063F948)` | 1 | **0** |

`FUN_180062940` is entry 11 of the 281. The `_initterm_e` range is entirely zero — an empty
C++-destructor range — so every initializer runs through the C-style table.

The thunks are IAT jump thunks and are absent from `_index.tsv`, which is why this was not
obvious from the decompiled tree:

| Thunk | IAT slot | Import |
| ----- | -------- | ------ |
| `0x180533E10` | `0x180644B00` | `api-ms-win-crt-runtime-l1-1-0.dll!_initterm` |
| `0x180533E20` | `0x180644B08` | `api-ms-win-crt-runtime-l1-1-0.dll!_initterm_e` |
| `0x180533E30` | `0x180644B10` | `api-ms-win-crt-runtime-l1-1-0.dll!_register_onexit_function` |

## The number that matters

`_callers.tsv` **omits** functions with no callers entirely rather than listing them with a
count of zero. Absence from that file therefore *means* zero, which is a trap: a lookup that
treats a missing key as "unknown" reports zero orphans and hides the whole effect.

| Set | Count |
| ---- | ----: |
| Functions decompiled (`_index.tsv`) | 18,825 |
| With ≥ 1 direct caller (`_callers.tsv`) | 7,822 |
| **With zero direct callers** | **11,003** |
| — of those, provably run at every DLL load | **280** |
| Init entries not decompiled (thunks/library) | 230 of 281 |
| Init entries also reached by a direct `call` | 1 (`0x1803EA850`) |

**280 functions in this binary have no direct caller and demonstrably execute on every load.**
So for the 11,003 "zero caller" functions, absence of a caller is *not* evidence of dead code,
and must not be used to prune, merge or dismiss a function.

## Consequence for feature mapping

`FUN_180062940` is a **DLL-load static constructor**. Its 77 UI names are registered at load
time, with no call site in Mega Hack's own code anywhere — which is also why the 77 UI names
and the 138 numeric settings share no function: registration and application are separate
phases, separated by load time rather than by a missing call.

This does not by itself locate the settings bridge. It does mean the bridge cannot be found by
looking for a function that both sides mention.

## Verification

- `scripts/test_init_table.py` pins the two ranges, the 281/0 entry counts, and the presence of
  `FUN_180062940` at slot 11. It skips when no sample set is present.
- Two independent derivations agree: the table was first found by scanning `.rdata` for the
  function's own VA, and `scripts/init_table.py` re-derives the whole structure from the
  import table without being given either address.

## Open

- Where the UI key space is bridged to the numeric settings at runtime.
- The single init entry that also has a direct caller (`0x1803EA850`), which is probably a
  deliberate explicit re-run but is unexamined.

## The other indirect set: hook bodies, and a hypothesis that failed

The same question applies to Mega Hack's 277 hook bodies in `_wrappers.tsv`, and it was worth
asking before trusting any "unreachable function" claim elsewhere.

| Set | Count |
| ---- | ----: |
| Hook bodies | 277 |
| With ≥ 1 direct caller | **10** |
| With zero direct callers | **267** |
| Zero-caller bodies with any RIP-relative reference | **0** |

Disassembly confirms these are real hook bodies rather than stubs — stack-canary setup, then a
tail `jmp` through a stored original-function pointer:

```asm
0x18010de30  push  rbp / sub rsp,0x30 / lea rbp,[rsp+0x30]
0x18010de3a  mov  qword ptr [rbp-8], -2
0x18010de42  mov  eax, dword ptr [rip+...]      ; stack cookie
0x18010de4e  mov  rdx, qword ptr gs:[0x58]      ; CookieBase
0x18010de6f  jmp  qword ptr [rip + 0x6f3ee1]    ; the original function
```

So 267 hook bodies are reached by a mechanism neither the call graph nor a RIP-relative scan can
see, which is the same blind spot as `_initterm` but larger.

### The RVA-table hypothesis, and why it is rejected

The obvious hypothesis: registration stores **RVAs** rather than VAs, which would be
ASLR-safe and consistent with the crashes recorded in `AGENTS.md`. A 4-byte search for every
one of the 267 body RVAs in `.rdata` matches **267 of 267**.

That match is not believed, and the reason is worth recording:

1. The hits span **1,428,418 bytes** of `.rdata` with gaps of 104/108/112/248/252/256 — not the
   shape of a table, but of scattered coincidences.
2. The offsets are **not consistently aligned** (e.g. `0x1F912` and `0x149104` are 2 and 4
   past an 8-byte boundary).
3. Dumping the bytes at the first hit shows **coherent x86-64 machine code**, not a record:
   `48 c7 45 ff fe 18 45 c7` is `mov qword ptr [rbp-1], 0xC74518FE`, `4d 8b 48 04` is
   `mov r9, qword ptr [r8+4]`. `.rdata` is `0x40000040` — readable, **not executable** — so this
   is stored code, plausibly original bytes saved for unhooking.

A 4-byte value match inside stored instruction bytes is exactly the false positive an
unaligned substring search produces. The address arithmetic was re-checked with
`scripts/pe_rva.py` (`0x18067D018` resolves to `backed`, `.rdata`, note `0x149018 into
.rdata`), so this is **not** an RVA/offset mapping bug — the mapping is right and the
interpretation is wrong.

Recorded as a rejected approach so the next investigation does not spend the hour again.
UNVERIFIED: how the 267 bodies are registered. It is not a RIP-relative reference, not a direct
call, and not an aligned `.rdata` RVA table.