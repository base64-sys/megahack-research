# Module entry and the early-load path

- **Question** — what is Mega Hack's module entry (`geodeImplicitEntry`), what does it do
  at load, and in what order do the early-load callbacks (tab registration, the
  `CORE` readiness probe, the `Import .dt` tag resolution) run?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`
  (see [provenance.md](provenance.md)).
- **Method** — `tools/pe_disasm.py` and `tools/pe_xrefs.py` on the binary (run from
  `samples/absolllute.hackmega/`), plus the Ghidra decompilation under
  `recovered/functions/` (`build_callgraph.py`, `_rtti.tsv`). Two independent
  angles are cited per claim: (A) raw disassembly and (B) decompiled pseudocode
  / RTTI / xref index.

## Evidence — `geodeImplicitEntry` itself

VA `0x180072B60` (RVA `0x72B60`), exported symbol `geodeImplicitEntry`, C linkage,
152 bytes. Angle A — disassembly (see [hackpro-api.md](hackpro-api.md#second-angle--addresses-land-on-real-code)):

```
0x180072B60  sub  rsp, 0x68
0x180072B64  mov  rax, [rip + 0x7844D5]        ; __security_cookie
...
0x180072B73  mov  rcx, [rip + 0x78E396]        ; DAT_180800F10 (mod slot)
0x180072B7A  test rcx, rcx
0x180072B7D  je   0x180072BC6                  ; first load: take next mod
0x180072B7F  call [rip + 0x5D0223]              ; IAT: geode::Mod::isInternal
0x180072B85  test al, al
0x180072B87  jne  0x180072B97                  ; internal mod -> skip async
0x180072B89  call [rip + 0x5D0259]              ; IAT: geode::async::runtime
0x180072B8F  mov  rcx, rax
0x180072B92  call 0x1800FFF60                  ; FUN_1800FFF60(runtime)
0x180072B97  lea  rax, [rip + 0x4CF8EA]        ; -> 0x180542488
0x180072B9E  mov  [rsp+0x20], rax
0x180072BA3  lea  rcx, [rsp+0x20]
0x180072BA8  call 0x1800FDCE0                  ; FUN_1800FDCE0(&descriptor)
0x180072BAD  ... stack-cookie check ...
0x180072BC5  ret
0x180072BC6  call [rip + 0x5D0184]              ; IAT: geode::Loader::get
0x180072BCC  mov  rcx, rax
0x180072BCF  call [rip + 0x5D024B]              ; IAT: geode::Loader::takeNextMod
0x180072BD5  mov  [rip + 0x78E331], rax        ; DAT_180800F10 = mod
0x180072BDF  call [rip + 0x5D01C3]              ; IAT: geode::Mod::isInternal
0x180072BE5  test al, al
0x180072BE7  je   0x180072B89                  ; non-internal -> async path
0x180072BE9  jmp  0x180072B97                  ; internal -> descriptor install
```

Angle B — decompiled pseudocode (`recovered/functions/180072b60_geodeImplicitEntry.c`):

```c
local_8 = __security_cookie ^ auStack_68;
if (DAT_180800F10 == 0) {
    this = geode::Loader::get();
    DAT_180800F10 = (longlong)geode::Loader::takeNextMod(this);
    bVar1 = geode::Mod::isInternal((Mod *)DAT_180800F10);
    if (bVar1) goto LAB_180072B97;
} else {
    bVar1 = geode::Mod::isInternal((Mod *)DAT_180800F10);
    if (bVar1) goto LAB_180072B97;
}
pRVar2 = geode::async::runtime();
FUN_1800FFF60(pRVar2);
LAB_180072B97:
local_48[0] = &PTR_FUN_180542488;   // 0x180542488
FUN_1800FDCE0(local_48);
... cookie check ...
```

Conclusion: `geodeImplicitEntry` is the Geode loader callback. It obtains Mega Hack's `Mod*`
once (cached in `DAT_180800F10`), and for a non-internal mod runs `FUN_1800FFF60`
against `geode::async::runtime()` (an async task / coroutine entry — UNVERIFIED which).
It then installs a descriptor whose first field is `0x180072C00` (`FUN_180072C00`,
the function immediately following the entry) through `FUN_1800FDCE0`. The descriptor
lives at `0x180542488` in `.rdata` and is 8 bytes of pointer + zero fill.

UNVERIFIED: the identity of `FUN_1800FFF60` and of the descriptor's consumer. Confirming
needs `DumpDecompileFile.java` on `0x1800FFF60` and callers of `DAT_180801708`.

## Evidence — `FUN_1800FDCE0` (descriptor install)

VA `0x1800FDCE0`, 217 bytes, **one static caller** (`geodeImplicitEntry`). Angle B:

```c
ppuVar3 = (undefined **)*param_1;          // *descriptor
if (param_1 != &DAT_180801708) {
    ppuVar4 = DAT_180801708 (or &PTR_FUN_18054CE70 if zero);
    if (ppuVar3 == 0) ppuVar3 = &PTR_FUN_18054CE70;
    pcVar1 = ppuVar3[1];                   // descriptor[1]
    pcVar2 = ppuVar4[2];                   // global[2]
    if (pcVar1 == 0) {
        if (pcVar2 != 0) (*pcVar2)(&DAT_180801708);   // invoke global's slot 2
        // move *param_1 into DAT_180801708
        DAT_180801708 = *param_1 (as two dwords + two dwords);
    } else if (pcVar2 == 0) { ... }
}
```

So `DAT_180801708` is a global descriptor slot that is populated once from the
entry's descriptor, with a vtable-style callback at slot 2 of the previous occupant
being invoked before overwrite. This is a one-shot install of a callback table.

## Evidence — the early-load callback chain

`pe_xrefs.py` shows `FUN_180002240` (VA `0x180002240`) is referenced by a single
`lea rax, [rip - 0x70888]` at `0x180072AC1`, inside `FUN_180072AA0`
(VA `0x180072AA0`). Angle B (`recovered/functions/180072aa0_FUN_180072aa0.c`):

```c
uVar1 = FUN_180001010();          // &DAT_1808004D0 (CoreDirector singleton)
FUN_180002200(uVar1);             // conditional init (+0x381 flag)
local_48 = std::_Func_impl_no_alloc<...Binder<CoreDirector::*, CoreDirector*>>::vftable;
local_40 = FUN_180002240;        // callable target
local_38 = 0;
local_10 = &local_48;             // std::function storage
local_30 = uVar1;                 // bound CoreDirector*
FUN_1800BAF30();                  // invoke the std::function
```

So `FUN_180072AA0` builds a `std::function<void()>` bound to `FUN_180002240(CoreDirector*)`
and invokes it immediately. `FUN_1800BAF30` (VA `0x1800BAF30`) has **one static caller**
(`FUN_180072AA0`) and is the invoke-and-destroy helper for that std::function.

`FUN_180002240` (VA `0x180002240`, 178 bytes) — Angle B:

```c
FUN_1800D96D0(param_1, 0);
uVar3 = FUN_1800BBD90();          // game state probe
cVar2 = FUN_1800BBEC0(uVar3);    // ready?
if (cVar2 != '\0') {
    FUN_180002320();              // tab registration + CORE probe
}
// writes the string "FIRST_LOAD" (0x4F4C5F5453524946 / 0x4441) into param_1
FUN_1800DDD00(param_1, 1, local_38);
```

So `FUN_180002240` is a hook callback (it receives a `param_1`) that, when the game
reports ready, calls `FUN_180002320` (the tab-registration + `CORE` readiness sequence
documented in [tab-registration.md](tab-registration.md)) and records a `"FIRST_LOAD"`
marker. If not ready, it does not register tabs.

`FUN_180006070` (VA `0x180006070`, 386 bytes) — Angle B:

```c
cVar3 = *(char *)(param_1 + 0x381);          // "already loaded" flag on CoreDirector
uVar5 = FUN_1800BBD90();
if (cVar3 == '\0') {
    cVar3 = FUN_1800BBEC0(uVar5);
    if (cVar3 != '\0') {
        FUN_1800AFAE0(); FUN_1800A8020();
        *(param_1 + 0x381) = 1;
        FUN_180002320();                    // tab registration + CORE probe
        FUN_180001B00(param_1, 1, *(param_1+0x390), 1);
        goto ...;
    }
    // otherwise bind FUN_180002620 as a deferred std::function on CoreDirector
    pcStack_70 = FUN_180002620;
    ...
    FUN_1800BC0B0(uVar5);                  // schedule the deferred call
}
```

So `FUN_180006070` is the "load once" path: guarded by `CoreDirector +0x381`, it
initialises, sets the flag, and calls `FUN_180002320`; if the game is not ready it
defers `FUN_180002620`.

`FUN_180002620` (VA `0x180002620`, 58 bytes) — Angle B:

```c
FUN_1800AFAE0(); FUN_1800A8020();
*(param_1 + 0x381) = 1;
FUN_180002320();                            // tab registration + CORE probe
FUN_180001B00(param_1, 1, *(param_1+0x390), 1);
```

i.e. the deferred realisation of the same load.

`FUN_180002320` itself (VA `0x180002320`, 537 bytes) is the tab-registration function
documented in [tab-registration.md](tab-registration.md): it registers the two tabs
(`HOME` at `0x180002350`, `KEYBINDS` at `0x180002372`), runs the TLS-guarded
initialiser at `DAT_1808008D8` (constructing `DAT_1808004D0` with `atexit(FUN_180001370)`),
and performs the `"CORE"` readiness probe at `0x1800023DC` (`FUN_1800A2FB0`).

## Evidence — how the callbacks are reached (virtual dispatch)

`pe_xrefs.py` reports **no static code references** to `FUN_180072AA0` or
`FUN_180006070`. They are reached through data:

- `0x180072AA0` is stored as a qword at VA `0x1805423C8` in `.rdata`. That slot sits
  inside a vtable whose RTTI name (from `_rtti.tsv`) is
  `std::_Func_impl_no_alloc<lambda_1 in operator() of lambda_0 in setup(), void, NXDirector*>`
  (vtable base `0x1805423B8`, COL at `0x1805423F0`). So `FUN_180072AA0` is the body of a
  lambda created inside a function named `setup()`, registered as a `std::function<void(NXDirector*)>`.
- `0x180006070` is stored as a qword at VA `0x1805344C8`, which lies inside **CoreDirector's
  primary vtable** (`vtable_va 0x180534428`, COL `0x180534660`, RTTI `.?AVCoreDirector@@`),
  at slot offset `0xA0` (index 20, 0-based). Confirmed by `pe_rva.py`: `0x180534420` reads
  back `0x180534660` (the COL). So `FUN_180006070` is CoreDirector virtual method #20.

Angle A for the CoreDirector claim: the RTTI table row
`0x180534428  0x180534660  CoreDirector  .?AVCoreDirector@@  0` plus the byte dump
showing `0x180534420 = 0x180534660`. Angle B: the decompiled body of `FUN_180006070`
operates on `param_1` with `+0x381`/`+0x390` fields and binds `CoreDirector::*` members
(`FUN_180002620` is stored as `void (__cdecl CoreDirector::*)(void)`).

## Evidence — shutdown side (for completeness)

The early-load initialiser at `DAT_1808008D8` (TLS guard) constructs `DAT_1808004D0`
via `FUN_1800010D0` and registers `atexit(FUN_180001370)`. `FUN_180001370` (VA
`0x180001370`) resets CoreDirector vtables and frees two allocations — the release-side
shutdown path noted in [m1-residuals.md](m1-residuals.md). TLS callbacks: none (confirmed).

## Conclusion

1. The mod entry is `geodeImplicitEntry` (VA `0x180072B60`): it caches the `Mod*`,
   runs an async task for non-internal mods, and installs a descriptor through
   `FUN_1800FDCE0`.
2. Tab registration (`FUN_180002320`) is **not** called from the entry directly. It is
   invoked lazily by two early-load callbacks — `FUN_180002240` (when the game reports
   ready) and `FUN_180006070` (the guarded "load once" path, deferred to
   `FUN_180002620`) — both of which gate on `FUN_1800BBEC0` (a game-state readiness
   probe) and both of which call `FUN_180002320`.
3. `FUN_180002320` is the single convergence point: two tabs + TLS initialiser +
   `"CORE"` readiness probe.
4. `FUN_180072AA0` is a lambda body in `setup()` that drives `FUN_180002240` via a
   `std::function`; `FUN_180006070` is CoreDirector virtual #20.

## Open questions / UNVERIFIED

- UNVERIFIED: the identity of `FUN_1800FFF60` and the descriptor consumer
  (`DAT_180801708`). Confirm with `DumpDecompileFile.java`.
- UNVERIFIED: which Geode hook registers `FUN_180002240` / `FUN_180006070` as callbacks
  (they have zero static callers). Confirm via `_hook_sites.tsv` / the registration
  tables; the vtable slots above are the static evidence.
- UNVERIFIED: the function literally named `setup()` (`??setup@@YAXXZ`) that encloses the
  lambda; only its RTTI name survives. Confirm with a string/RTTI-to-VA pass.
- UNVERIFIED: `FUN_1800BBEC0`/`FUN_1800BBD90` semantics (the "ready" predicate). Confirm
  via decompilation and callers.
- The M2 exit criterion "reconstructed as compiling C++" is **not** yet met: this note
  documents the path and its VAs; a compilable `geodeImplicitEntry` reconstruction
  still has to be written against recovered host declarations.
