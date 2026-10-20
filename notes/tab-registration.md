# Tab registration and the `.data` zero-fill pointer table

- **Question** — `hackpro::extensions::registerTab` takes a `std::istream&` and a
  `Delegate*`. Where does that stream come from, and what is the serialised tab format?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

**Answer up front:** Mega Hack registers exactly **two** tabs, both assembled at load time
from in-memory streams built out of embedded constants. The *wire format* of those streams
was not determined — see [Open questions](#open-questions). What follows is the call
structure, which is fully verified.

## Method

- `tools/pe_disasm.py` at the export, at both call sites, and at the real implementation.
- `tools/pe_disasm.py --callers` to enumerate callers of the export.
- A `capstone` pass over all of `.text` collecting every rip-relative reference into
  `.data`'s zero-fill tail, to find who *writes* the slots the hook site reads.
- Section bounds read directly from `pefile` rather than inferred, after a wrong
  assumption about where those slots came from (see [Failure modes](#failure-modes-recorded)).

## Evidence — the export is a thin forwarder

`registerTab` at `0x180072E50`:

```
0000000180072e50  push rsi
0000000180072e51  push rdi
0000000180072e52  sub  rsp, 0x28
0000000180072e56  mov  rsi, rdx          ; rsi = Delegate*
0000000180072e59  mov  rdi, rcx          ; rdi = std::istream&
0000000180072e5c  call 0x1800a2720       ; accessor A
0000000180072e61  mov  rcx, rax
0000000180072e64  call 0x1800a2f80       ; accessor B
0000000180072e69  mov  rcx, rax
0000000180072e6c  mov  rdx, rdi          ; istream&
0000000180072e6f  mov  r8, rsi           ; Delegate*
0000000180072e72  add  rsp, 0x28
0000000180072e76  pop  rdi
0000000180072e77  pop  rsi
0000000180072e78  jmp  0x1800a60b0       ; tail call: the real implementation
0000000180072e7d  int3
```

So the exported symbol does no work itself: it fetches two singletons (almost certainly
`hackpro::extensions::instance()` and a related handle — note `0x1800A2720` is also called
directly at `0x180002377`, and `0x1800A2F80` returns what is then passed as `this`) and
tail-jumps to `0x1800A60B0` with `(this, istream&, Delegate*)`. Nothing is skipped or
adjusted; the epilogue is a plain tail call, so there is no caller-visible stack
obligation beyond the two pops.

UNVERIFIED: the identities of `0x1800A2720` and `0x1800A2F80`. Confirming needs
`DumpDecompileFile.java` on both. `0x1800A2F80` appears as the `rcx` producer immediately
before the `call 0x1800023DC` site discussed below, which is consistent with a singleton.

## Evidence — exactly two tabs

`pe_disasm.py --callers 0x180072e50` finds precisely two call sites: `0x180002350` and
`0x180002372`. Both are inside one function starting at `0x180002320`:

```
0000000180002320  push rbp
0000000180002321  push rsi
0000000180002322  sub  rsp, 0x68
0000000180002326  lea  rbp, [rsp + 0x60]
000000018000232b  mov  qword ptr [rbp], 0xfffffffffffffffe      ; EH state

; --- tab 1 ---
0000000180002333  call 0x1800bea70
0000000180002338  lea  rdx, [rbp - 0x20]
000000018000233c  mov  rcx, rax
000000018000233f  call 0x1800bed70                              ; build stream 1
0000000180002344  call 0x180039360                              ; delegate 1
0000000180002349  mov  rcx, qword ptr [rbp - 0x20]              ; stream 1
000000018000234d  mov  rdx, rax
0000000180002350  call 0x180072e50                              ; registerTab

; --- tab 2 ---
0000000180002355  call 0x1800bea70
000000018000235a  lea  rdx, [rbp - 0x10]
000000018000235e  mov  rcx, rax
0000000180002361  call 0x1800bee20                              ; build stream 2
0000000180002366  call 0x18003d220                              ; delegate 2
000000018000236b  mov  rcx, qword ptr [rbp - 0x10]              ; stream 2
000000018000236f  mov  rdx, rax
0000000180002372  call 0x180072e50                              ; registerTab
```

Both tabs share the same handle producer (`0x1800BEA70`) but use **different** stream
builders (`0x1800BED70` / `0x1800BEE20`) and **different** Delegates (`0x180039360` /
`0x18003D220`). So the two tabs are distinct definitions with distinct renderers, not one
definition reused.

UNVERIFIED: that these are the only tabs. Two *static* call sites is strong but not
conclusive — a tab could be registered from an indirect call through a function pointer.
Confirming needs a scan for `call [reg]`/`call [rip+…]` targets landing on `0x180072E50`
or `0x1800A60B0`.

## Evidence — the documented hook sits in the same function

`0x1800023DC`, labelled in `RE_NOTES_PERSISTENT_BEST_RUN.md` as "resolve the native import
action tag after Mega Hack core load", is `0x8C` bytes past the second `registerTab` call, in
the same function. So tab registration and the `Import .dt` tag resolution are one
load-time sequence. That is new context the RE notes do not record.

The hook site and its callee:

```
00000001800023c3  call 0x1800bea70
00000001800023c8  lea  r8, [rip + 0x7fe471]     ; -> VA 0x180800840
00000001800023cf  lea  r9, [rip + 0x7fe472]     ; -> VA 0x180800848
00000001800023d6  mov  rcx, rsi
00000001800023d9  mov  rdx, rax
00000001800023dc  call 0x1800a2fb0              ; <-- documented hook
00000001800023e1  test al, al
00000001800023e3  je   0x180002457
```

and `0x1800A2FB0` builds a small-string-optimised `std::string` containing `"CORE"`:

```
00000001800a3002  mov  qword ptr [rbp + 0x20], 0x4      ; size  = 4
0000000180003008  mov  qword ptr [rbp + 0x28], 0xf      ; capacity = 15 (SSO)
00000001800a300a  mov  dword ptr [rbp + 0x10], 0x45524f43 ; "CORE"
```

`0x45524F43` is `"CORE"` as a little-endian dword (`43 4f 52 45` = `CORE`), with capacity 15
meaning the MSVC SSO buffer — so no heap allocation, the string is a stack temporary. It then
does `mov rax, [rax]; cmp rax, qword ptr [rip + 0x75e424]`, comparing a looked-up value
against a global at VA `0x180801410`, and returns a `bool`.

This reads as: *"is the thing named `CORE` present and equal to the expected value?"* — a
readiness probe, consistent with the RE notes' "after Mega Hack core load" wording.

UNVERIFIED: the full semantics. Confirming needs `DumpDecompileFile.java` on `0x1800A2FB0`
and on `0x1800A3880` (the lookup called twice inside it).

## Evidence — `.data`'s zero-fill tail is a runtime pointer table

This is the finding that changed my mind mid-investigation, and it generalises.

The two slots the hook site loads into `r8`/`r9` sit at RVA `0x800840` and `0x800848`.
Per `pefile`, `.data` is:

| Quantity | Value |
| -------- | ----- |
| `.data` RVA range | `0x6DF000`–`0x81BD90` |
| `.data` raw-backed RVA | `0x6DF000`–`0x800600` |
| `.data` file range | `0x6DE200`–`0x7FF800` |

`0x800840` is `0x240` **past** the raw-backed end, so it has no file bytes at all — it is
loader zero-fill. I initially assumed the two `lea`s pointed at adjacent string literals,
which would have made them static data. A scan for rip-relative references into
`0x800600`–`0x800900` settles it:

```
0x180001389  mov qword ptr [rip + 0x7ff4b0], rax   -> RVA 0x800840   <-- WRITE
0x180001397  mov qword ptr [rip + 0x7ff4aa], rax   -> RVA 0x800848   <-- WRITE
```

Both slots are **written** by an early initialiser at `0x180001389` / `0x180001397`, then
only ever read. So they are 8-byte runtime-populated pointers — module or object handles,
not strings. My "adjacent literals" reading was wrong.

The same scan found:

```
0x1808008D8   ~15 references, pattern  lea rcx,[rip+X] ; cmp dword ptr [rip+Y], -1
```

`lea rcx, [rip+…]` followed by `cmp dword ptr [rip+…], -1` is the MSVC dynamic
initialisation guard for a `__declspec(thread)` variable. `0x1808008D8` is therefore a TLS
guard slot, and `0x180002320` is a function that runs after TLS init — which is consistent
with it being load-time tab setup.

So `.data`'s zero-fill tail is **not** unused padding. Mega Hack keeps a live pointer table
there, populated by an initialiser around `0x180001300`–`0x180001400` and read from
throughout the binary. The last 64 bytes of `.data`'s raw data are all zero, which is why
nothing about this shows up when reading the file.

This connects to [provenance.md](provenance.md#evidence--mutable-state-block): the mutable
state block at `0x18081BFC0` also lives in that zero-fill tail, and for the same reason — it
is writable and zero *because of page rounding*, not because the section declares room for
it. Any tool that reasons about `.data` by `VirtualSize` will conclude that all of this is
unmapped and be wrong.

## Conclusion

1. `registerTab` is a forwarder to `0x1800A60B0`; the export carries no logic.
2. Exactly two tabs are registered statically, at `0x180002350` and `0x180002372`, from two
   distinct stream builders and two distinct Delegates.
3. Tab registration and the documented `0x1800023DC` hook are one load-time sequence in the
   function at `0x180002320`. That relationship is new.
4. The call at `0x1800023DC` is a `"CORE"` readiness probe returning `bool`, not a definition
   registration. Earlier assumption corrected.
5. `.data`'s zero-fill tail holds a runtime-populated pointer table and a TLS guard. Reading
   it as padding produces wrong conclusions.
6. The tab serialisation format itself is still unknown.

## Open questions

- (Resolved 2026-10-20.) `.rsrc` is only `0x18C` bytes, far too small to hold tab
  definitions, so the tab data is code-embedded rather than a resource. `FUN_1800BED70`
  wraps the blob at `DAT_1806FE004` (size `0x2D5`) and `FUN_1800BEE20` wraps the blob at
  `DAT_1806FE2D9` (size `0xC2`), both as `CompressedBuffer` (see rtti-classes.md). Both
  are compressed, so the grammar is not readable as text without decompression. UNVERIFIED:
  the decompression routine and the uncompressed grammar.
- (Resolved 2026-10-20.) `0x180039360` is the `mega::extensions::Home` → `core::HomeWin`
  singleton (vtable at `0x180800AD8`); `0x18003D220` is the `core::Keybinds` singleton
  (vtable at `0x180800C20`). Both are thread-safe static initialisers with `atexit`
  destructors, so they are long-lived service objects, not per-tab instances. UNVERIFIED:
  which methods on those vtables a third-party tab must implement.
- Prove two tabs is exhaustive by looking for indirect calls into `0x180072E50` /
  `0x1800A60B0`.
- (Resolved 2026-10-20.) `0x1800A2720` is the registry singleton accessor (returns
  `DAT_180800F58`, constructing it via `FUN_1800A27E0` on first use); `0x1800A2F80` is a
  trivial `param_1 + 8` helper, not a singleton. UNVERIFIED: why the second is passed as a
  Delegate where the first is the registry.

## Failure modes recorded

- **Assumed the two `lea`s addressed adjacent string literals.** They address 8-byte
  pointers in `.data`'s zero-fill tail. The error came from reasoning about an address
  without first asking what section actually backs it. Always resolve the containing
  section and check the RVA against **both** `VirtualSize` and `SizeOfRawData` before
  treating a file offset as meaningful.
- **`pefile.get_offset_from_rva(0x800840)` returns `0x7FFA40`**, which is inside `.pdata`'s
  raw data — unwind information, not the requested address. `pefile` maps linearly and does
  not refuse RVAs past a section's `SizeOfRawData`. Do not trust it for zero-fill RVAs.
- A throwaway script printed `.reloc`'s section flags under a `.data` label because of a
  stale loop variable; the real `.data` characteristics are `0xC0000040` (read/write).