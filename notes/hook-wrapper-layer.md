# The runtime hook-wrapper indirection layer

The single biggest obstacle to reading the recovered tree, and how to get past it.

- **Question** — why does following a call in `recovered/functions/` so often hit an
  identical-looking thunk, and where does it actually go?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/resolve_wrappers.py` → `recovered/functions/_wrappers.tsv`.

## The shape of the problem

Three functions in a row while chasing the New Best label all looked like this:

```c
HMODULE base = GetModuleHandleA(NULL);        // FUN_18010ddc0
auto  fn    = FUN_1801105e0(base + 0x132e50);  // createWrapper
fn(args);                                     // indirect call through a cached pointer
```

with a TLS guard around the initialisation. That is not three coincidences. It is one
pattern applied everywhere, and it is why `recovered/` is full of near-identical stubs.

## What it actually is

`FUN_18010DDC0` decompiles to the following and nothing else:

```c
HMODULE FUN_18010ddc0(void)
{
  ...
  DAT_180806ac0 = GetModuleHandleA((LPCSTR)0x0);
  ...
  return DAT_180806ac0;
}
```

`GetModuleHandleA(NULL)` returns the base address of the calling module — here the Mega Hack
DLL itself, `0x180000000`. So the `+ 0x132e50` is an **RVA from the image base**, and
`base + RVA` is the hook target.

The resolver, `FUN_1801105E0`, calls a Geode import:

```c
geode::hook::createConvention((TulipConvention)local_58);
FUN_1800aa110(local_38, 0, &local_80);
geode::hook::createWrapper(&local_80, param_1);
```

`geode::hook::createWrapper` is imported at IAT `0x180642D20` (see
[host-abi.md](host-abi.md)). Geode builds an executable trampoline for the target under
tulip's calling convention and hands back a callable pointer, which is cached in a global.
The TLS guard plus `_Init_thread_footer` is MSVC's thread-safe one-time initialisation.

So Mega Hack declares its hooks as **RVAs and binds them at load time through Geode**. The
callee is never a static call in the binary, which is exactly why a decompiler cannot
recover it.

## Scale

`scripts/resolve_wrappers.py` over the whole tree:

```
recovered functions scanned : 18825
thunk functions             : 279  (call GetModuleHandleA)
hook sites                  : 277
distinct hook targets       : 276
targets inside a function   : 266
```

- The 279 thunks are **contiguous**, spanning roughly `0x18010DE30`–`0x180111200`. One
  generated block, not scattered code.
- All 276 targets are in `.text`, RVA `0x3C020`–`0x4CE150`.
- **266 of 276 land inside a recovered function**, at mid-function offsets — `+0xDA3`,
  `+0x1F0`, `+0x60`. That is the expected shape for a hook: patch the middle of a
  function, not its entry.
- The remaining 10 land in padding between functions or at addresses Ghidra did not
  resolve.

`_wrappers.tsv` gives one row per site: thunk, resolver, target RVA and VA, and the
containing function with the offset. 276 of Mega Hack's hooks are now individually
addressable, where before they were invisible.

## Decompiler trust, a concrete failure

Ghidra typed the resolver's parameter as `WrapperMetadata *param_1`, which makes
`base + RVA` look like a *metadata record*. The section analysis supported that reading
briefly. It is wrong.

The disassembly shows `local_80 = 8` immediately before the call. `&local_80` is the
WrapperMetadata being constructed on the stack, and `param_1` — `base + RVA`, always in
`.text`, always at a mid-function offset — is the **target**. The decompiler had the
argument roles inverted, which is a clean instance of the AGENTS.md rule: never trust
Ghidra's types or argument order when it disagrees with the section table and the
displacement pattern.

Getting this backwards produced a plausible but entirely false story ("the target is
encoded in static metadata and is therefore opaque"). It is not opaque; it is a plain
RVA.

## Why this matters for the reconstruction

This bounds what any static reconstruction can do, and it is worth stating plainly:

- ~276 call sites cannot be followed without either running the mod or accepting the RVA
  table above. The table is the mitigation.
- The hooks are Mega Hack hooking **itself** (`FUN_18011FFE0` and neighbours are the
  targets), i.e. its features patch its own code rather than calling through vtables. That
  is why the export table looked so thin: most behaviour is behind this layer.
- Because the layer is uniform and generated, resolving all 277 sites mechanically is
  strictly better than doing it by hand, which is why `scripts/resolve_wrappers.py` exists.

UNVERIFIED: whether any of the 276 targets point outside the Mega Hack image. All 277 fall
inside `.text` of this module, but a target could in principle be a fixed address in
`GeometryDash.exe`; that would show up as an RVA that happens to land in `.text` here,
which static analysis cannot distinguish. Confirming needs a runtime read of a wrapper
pointer.

## Next step this unlocks

The New Best draw routine is now reachable. `FUN_180110790` binds target `0x180132E50`,
which resolves to `FUN_180132CE0 + 0x170` — a real function body in the recovered tree.
Reading it, together with `FUN_180149670`'s target for row `0095`, is what will settle
whether the session-vs-global distinction is a value comparison, per
[definition-keys.md](definition-keys.md#the-negative-result).

## Failure modes recorded

- **Recovered filenames carry the full preferred-base VA, not the RVA.** `180169eb0_FUN_…`
  is VA `0x180169EB0`. Parsing that as an RVA silently misaligns every range lookup, which
  made all 276 targets report as "not inside any function" on the first run. Subtract the
  image base before comparing against `base + RVA` values.
- Iterators cannot be concatenated with `+` in Python; `finditer` returns an iterator, not
  a list.
- A loose regex matching `identifier + 0xNN` swept up ordinary member offsets like
  `this + 0x20`, producing 4,625 false sites. Anchor on the variable actually assigned from
  the base call.