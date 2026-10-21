# Hook dispatch: how Mega Hack reaches Geometry Dash

- **Question** — Mega Hack imports zero GD gameplay symbols. How does it actually call GD
  code, and can the mechanism be reproduced rather than reverse-engineered hook by hook?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-18.
- **Tools** — `_host_methods.tsv`, `_wrappers.tsv`, `_callers.tsv`, and the recovered `.c`
  for each function named below.

## The answer in one line

Mega Hack takes `GetModuleHandleA(NULL)` — the Geometry Dash executable's own base — and
treats **hard-coded RVAs into it** as addresses of GD functions, wrapping each in a Geode
hook under a convention it calls `TulipConvention`.

## Step 1 — the host base

`FUN_18010DDC0`, VA `0x18010DDC0`, RVA `0x10DDC0`, 109 bytes:

```
HMODULE FUN_18010ddc0(void)
{
  if (*(int *)(*(longlong *)(TLS + _tls_index * 8) + 0x10) < DAT_180806ac8) {
      FUN_1803a5140(&DAT_180806ac8);
      if (DAT_180806ac8 == -1) {
          DAT_180806ac0 = GetModuleHandleA((LPCSTR)0x0);
          _Init_thread_footer(&DAT_180806ac8);
          return DAT_180806ac0;
      }
  }
  return DAT_180806ac0;
}
```

A thread-safe static initialiser around `GetModuleHandleA(NULL)`, cached in `DAT_180806AC0`.
With a NULL argument that returns the base of the **host executable**, i.e. Geometry Dash.

**This is the highest-fanout function in the binary: 278 direct callers.** For comparison,
`_wrappers.tsv` has 277 rows and `FUN_18010DDC0` is not one of them. Almost every "how does
Mega Hack touch the game" question ends here.

## Step 2 — the registration shape

`FUN_180199D10`, VA `0x180199D10`, RVA `0x199D10`, 213 bytes, one caller, is a pure
registration function — three hook installs and nothing else:

```
uVar2 = FUN_18010a370(FUN_18010a250(), 0x16c8b0);
FUN_18019a0c0(FUN_180108960(), uVar2, FUN_180199df0, "GJGameLevel::savePercentage", 0xffffffe0, 1);

uVar2 = FUN_18010a370(FUN_18010a250(), 0x3a7a80);
FUN_1800ab850(FUN_180108960(), uVar2, FUN_180199e30, "PlayLayer::levelComplete", 0xffffffe0, 1);

uVar2 = FUN_18010a370(FUN_18010a250(), 0x3b8eb0);
FUN_1800ab850(FUN_180108960(), uVar2, FUN_180199e80, "PlayLayer::resetLevel", 0xffffffe0, 1);
```

Six arguments: a resolver, a target, a callback, a **method name for diagnostics**, `0xFFFFFFE0`
(−32, Geode's "run before the original" ordering), and `1`.

Both registration helpers — `FUN_18019A0C0` and `FUN_1800AB850` — begin with

```
geode::hook::createConvention((TulipConvention)local_78);
```

so the hook convention is named in the debug type, not guessed. That is where the
reproduction starts: a `TulipConvention` plus Geode's `hook::createWrapper`.

## Step 3 — the lazy thunk, which is where the RVAs are confirmed

Each Mega Hack call to a *GD original* goes through a 147-byte MSVC-style lazy resolver.
`FUN_1801A4300`, VA `0x1801A4300`, RVA `0x1A4300`, **24 callers**:

```
_DAT_18080fe48 = (code *)FUN_1801a33c0(lVar1 + 0x3b8eb0);   // lVar1 = FUN_18010ddc0(...)
...
(*_DAT_18080fe48)();                                          // tail call
```

`FUN_1801A33C0` (VA `0x1801A33C0`, 325 bytes) calls `geode::hook::createConvention` and
`geode::hook::createWrapper`, returning a function pointer; the caller caches it and
tail-jumps.

Two independent angles confirm each RVA, and both are needed because either alone is weak:

| GD method | RVA | Angle 1: registration passes it | Angle 2: thunk reads it back |
| --------- | --: | -------------------------------- | --------------------------- |
| `PlayLayer::resetLevel` | `0x3B8EB0` | `FUN_180199D10` | `FUN_1801A4300`, 24 callers |
| `PlayLayer::levelComplete` | `0x3A7A80` | `FUN_180199D10` | `FUN_1801A5190`, 2 callers |
| `GJGameLevel::savePercentage` | `0x16C8B0` | `FUN_180199D10` | `FUN_1801462C0`, 1 caller |

The two angles are genuinely different: one is a *call argument* the registration helper
receives, the other is an *addend in a lazy initialiser* that independently re-derives the
same value. A typo in one would not survive the other.

`FUN_1801462C0`, the `savePercentage` thunk, is the same shape as the other two with two
differences worth recording:

```
uVar2 = param_2 & 0xffffffff;      // only the low 32 bits are preserved
uVar3 = param_3 & 0xffffffff;
uVar4 = param_4 & 0xffffffff;
...
DAT_180807770 = (code *)FUN_1801463a0(lVar1 + 0x16c8b0);
(*DAT_180807770)(param_1, uVar2, uVar3, uVar4);
```

It **truncates three arguments to 32 bits** before forwarding, so
`GJGameLevel::savePercentage` is called with 32-bit values — consistent with a
`(PlayLayer*, int, int, int)` shape. And it uses a *different* wrapper builder,
`FUN_1801463A0`, where the no-argument thunks use `FUN_1801A33C0`. Two builders for two
parameter shapes is further evidence that the convention is per-signature rather than a
single global setting.

## Step 4 — what the hooks actually do

Three callbacks, and they are small enough to read fully.

**`FUN_180199E80`**, the `resetLevel` hook — 19 bytes, the whole function:

```
void FUN_180199e80(void) {
  FUN_1801a4300();      // the original PlayLayer::resetLevel
  FUN_18019b330();      // Mega Hack's own follow-up
}
```

No guard, no state, no re-entrancy. It calls the original first, then its own work. This is
the cleanest possible example of the pattern a reconstruction has to match.

**`FUN_180199DF0`**, the `GJGameLevel::savePercentage` hook — 64 bytes:

```
cVar1 = FUN_18019b5f0();                  // a Mega Hack predicate
if (cVar1 != '\0') return;               // <-- BLOCKS the original entirely
FUN_1801462c0(param_1, param_2, param_3, param_4);   // the original
```

This one can **veto**. When `FUN_18019B5F0()` is true, GD never learns the percentage was
saved at all. That is a behavioural fact a reconstruction must not miss: it is not a
decorate-then-forward hook, it is a replace-or-block hook.

**`FUN_180199E30`**, the `levelComplete` hook — 67 bytes, with re-entrancy protection:

```
cVar1 = FUN_18019b5f0();
if (cVar1 != '\0' && *(char *)(param_1 + 0x3230) == '\0') {
    *(undefined1 *)(param_1 + 0x3230) = 1;
    FUN_1801a5190(param_1);          // the original
    *(undefined1 *)(param_1 + 0x3230) = 0;
    return;
}
FUN_1801a5190(param_1);
```

`PlayLayer+0x3230` is a **one-byte re-entrancy flag** Mega Hack sets around the original
`levelComplete`, so its own callbacks fired by that original can tell they are running
inside it. `FUN_18019B5F0()` is the shared predicate that gates all three hooks.

**Verified GD structure offset: `PlayLayer + 0x3230`.** Derived from the load/store pair
above, not from any struct layout assumption.

## The shared gate, `FUN_18019B5F0`

`FUN_18019B5F0`, VA `0x18019B5F0`, 117 bytes, 2 callers — the predicate all three hooks
consult. Read in full it is the mod's anti-cheat check, and it is short enough to state
exactly:

```
lVar2 = FUN_1801acdb0();                       // 50 callers
if (lVar2 != 0) {                              // ...there is a current level
    uVar3 = FUN_18008a510();
    if (FUN_18008f420(uVar3, 0x1e2))  goto ACTIVE;                  // config tag
    if (FUN_18008f420(uVar3, 0x1e1) && DAT_18080cbd8) goto ACTIVE; // config tag + safe mode
    if (FUN_18002de50(uVar3, 0x49) && FUN_180199520() >= 1.0) goto ACTIVE;
}
return INACTIVE;
```

with no current level meaning **inactive**, so the hooks pass GD's original through
untouched.

Its three parts:

- `FUN_1801ACDB0` (VA `0x1801ACDB0`, 21 bytes, **50 callers**) is
  `*(thunk_FUN_180147BA0() + 0x208)` — the current level pointer, with the null test being
  the "not in a level" guard.
- `DAT_18080CBD8` is a cached safe-mode flag. `FUN_18019B330`, VA `0x18019B330`, 20 bytes,
  one caller, is the whole writer:
  `DAT_18080CBD8 = mega::utils::safe_mode::areCheatsEnabled();`
  and it runs after **every** `resetLevel`, because `FUN_180199E80` calls it. So safe mode
  is re-evaluated once per level start, not per frame.
- `FUN_180199520` (VA `0x180199520`, 58 bytes) is a two-global float expression:
  `(1.0 - DAT_18080cb78 / DAT_18080cb70)` when `DAT_18080cb70 > 0.0`, else `1.0`. It is a
  **progress fraction** built from two `double` globals, and the gate fires when it reaches
  `1.0`.

`mega::utils::safe_mode::areCheatsEnabled` is one of the 30 exported symbols
(VA `0x18019B350`), so this whole scheme is reachable from other mods through the
extension API.

UNVERIFIED: what the two progress globals at `DAT_18080CB70` / `DAT_18080CB78` measure, and
why `>= 1.0` is the trigger rather than `>= 100.0` or a fraction of one. The expression
yields `1.0` both when the denominator is zero and when the numerator equals it, so at least
two different states collapse onto the same value and the comparison is disambiguating
between them by exclusion.

## The complete hook surface

`scripts/hook_sites.py` + `recovered/functions/_hook_sites.tsv`. The earlier version of this
script restricted the registration helper to two names (`FUN_1800AB850` and
`FUN_18019A0C0`) and reported 43 sites. The helper actually varies by callback signature
-- 143 distinct helpers, all calling `geode::hook::createConvention` -- so the full sweep
now yields **338 registration sites over 215 distinct GD methods**. The method name, the
callback and the host target all come from the registration call; the target is either a
hard-coded host RVA (317 sites) or a name resolved at runtime from a mangled host symbol
(30 sites over 26 methods).

Every method maps to exactly one target, with no method carrying two. That is the check
that the positional pairing is sound: 25 registrations of `resetLevel` all landing on
`0x3B8EB0` cannot be produced by a misaligned pairing. The three RVAs already confirmed by
the registration-plus-thunk route above -- `0x3B8EB0`, `0x3A7A80`, `0x16C8B0` -- all
reproduce exactly, so this is a third independent angle on the same three values; the
`onQuit` thunk at `FUN_1801A5260` reads `lVar1 + 0x3BABB0`, making **four**
doubly-confirmed RVAs.

`PlayLayer` accounts for 27 of the 215 methods and 92 of the 338 registrations, so the
level lifecycle is still the largest single cluster, but the surface also spans rendering,
input, audio, UI, editor and FMOD paths. Mega Hack hooks far more than the level lifecycle.

### `0x180174DE4` is the `onQuit` hook's call to the original

`FUN_180174DE0`, the `PlayLayer::onQuit` callback, is 20 bytes. `pe_disasm.py` on
`_original.dll`:

```
0000000180174de0  48 83 ec 28      sub    rsp, 0x28
0000000180174de4  e8 77 04 03 00  call   0x1801a5260     <-- the address in the README
0000000180174de9  31 c9           xor    ecx, ecx
0000000180174deb  48 83 c4 28      add    rsp, 0x28
0000000180174def  e9 0c 00 00 00  jmp    0x180174e00      <-- tail into Mega Hack's body
0000000180174df4  cc              int3
```

So `0x180174DE4` is precisely the call to the original `PlayLayer::onQuit`, and the hook
**tail-jumps** rather than calls its own follow-up. The README's description of that
address moved from the patch's label to a disassembly-backed fact.

43 of the 338 callbacks are Ghidra `LAB_` labels and 19 are data slots, so neither has a
recovered `.c` to read. Recorded as `callback_kind=label` / `data` rather than dropped.

## The settings accessor this note used to call "missing"

The body after the `onQuit` hook's tail jump calls `FUN_18002EE40(config, &local, 0x11d)`.
`FUN_18002EE40` is a settings accessor — it folds all eight bytes of the third argument
through FNV-1a-64 and probes a map at `param_1 + 0xC0` with the mask at `+0xD8`:

```
lVar5 = ((... FNV-1a-64 of param_3 ...) * 0x100000001b3 & *(ulonglong *)(param_1 + 0xd8)) * 0x10;
lVar4 = *(longlong *)(*(longlong *)(param_1 + 0xc0) + 8 + lVar5);
```

An earlier version of this note read the differing offsets as proof that this was "a second
registry rather than another accessor on the first", and concluded that
`scripts/settings_index.py` missed it, so **tag `0x11D` (285) was absent from
`_settings.tsv`** and the index jumped 284 → 292 across the gap.

**Both halves of that are now corrected**, in
[settings-registry.md](settings-registry.md#corrected-fun_18002ee40-is-getconfigvarhackprobytes-not-a-second-registry):

- `FUN_18002EE40` is `getConfigVar<hackpro::Bytes>` — the **sixth** type-aware accessor on
  the **same** registry. The literal it emits on a miss is the requested type's name
  (`hackpro::Bytes`), and the map it probes (sentinel node `object + 0x188`, bucket array
  `object + 0x198`, mask `object + 0x1B0`) lives inside that same registry object. The
  offsets differ from `getConfigVar<bool>`'s because the two readers use different bases
  over one object, not because the objects differ.

  **Correction (2026-10-21 audit).** An earlier draft of this line called the probed map
  "the MSVC `std::unordered_map<uint64_t, std::vector<unsigned int>>` that `FUN_1800A28C0`
  constructs at `object + 0x170`". Both halves are wrong: the bucket stride is 16 bytes
  (`shl r8, 4` at `0x18002EF03`), where MSVC's bucket array holds 8-byte node pointers, and
  the container the accessor actually reads starts at `object + 0x180`, not at `object +
  0x170`. It is Mega Hack's own chained FNV-1a-64 map — the same shape as the string-keyed
  lookup `FUN_18007CB60` (mask at `+0x88`, sentinel at `+0x60`, buckets at `+0x70`, same
  stride) — and its exact C++ type is UNVERIFIED. See
  [settings-registry.md](settings-registry.md#resolved-the-bytes-map-is-a-chained-fnv-1a-64-hash-map-inside-the-registry).
- `scripts/settings_index.py` now recognises it. `_settings.tsv` carries the row
  `285	getConfigVar<hackpro::Bytes>	1	180174e00_FUN_180174e00`, and the 284 → 292 gap is
  closed. `scripts/test_settings_index.py` pins that row.

The lesson this section originally recorded is kept, because it is the useful part: **an
index built from an assumed accessor list silently omits settings, with no error anywhere.**
The fix is to enumerate accessors from the binary, not to assume them.

UNVERIFIED: what tag 285 is — whether it is a game-mod setting or a hackpro-internal one.
The value lives in `hackpro.json`, which this repo does not read.

## What this means for the milestones

- **M1, "verified GD offsets"**: three GD function RVAs and one GD field offset are now
  evidence-backed rather than assumed, each from two independent angles. `GJGameLevel+0x148/
  +0x14C/+0x158/+0x198/+0x1B8` are now verified (see `notes/gd-structures.md`); `+0x2C4`
  and `+0x35C` remain unverified.
  Note that Mega Hack reaches GD through **absolute RVAs into the host image**, not through
  Geode's `node-ids` indirection — so a re-baselining to another GD build changes every
  offset in this table, and those offsets must be re-derived, never carried over.
- **M2, "hook dispatch mechanism recovered, so new hooks are mechanical rather than
  bespoke"**: this is the mechanism. Registering a hook is
  `(resolver, targetFromGDBase(rva), callback, name, -32, 1)` under a `TulipConvention`,
  and calling a GD original is `FUN_1801A33C0(FUN_18010DDC0() + rva)`.
- **M3**: `FUN_18019B330` (called after every `resetLevel`) and `FUN_18019B5F0` (the shared
  predicate) are the two chokepoints through which most feature code must pass. Mapping
  their callers is likely the cheapest route into the 160+ feature modules.

## Open questions

- The two progress globals in `FUN_18019B5F0`: `DAT_18080CB70` accumulates every
  `param_2` delta passed to `FUN_1801992B0`, and `DAT_18080CB78` accumulates the same
  delta only while `DAT_18080CB68 == 1` (safe-mode/cheat-active). `FUN_180199250`
  (called from the `resetLevel` hook `FUN_180199E80`'s follow-up path) zeroes both at
  level start. `FUN_180199520` returns `1.0 - cb78/cb70` when `cb70 > 0`, else `1.0`,
  and `FUN_180199560` multiplies the same expression by `100.0`. The gate therefore fires
  when the cheat-attributed share of total progress reaches 100%. `param_2` is the float
  delta passed to the `PlayLayer::postUpdate` hook, i.e. frame time, so the globals
  accumulate time spent in cheat-attributed frames. `DAT_18080CB68` is cleared every frame in
  `FUN_1801992B0` and is a one-frame-delayed copy of `DAT_18080CB69`; no assignment to
  `1` appears in the recovered tree, so its setter is not in the recovered set (or is
  reached through a pointer). UNVERIFIED: what sets `DAT_18080CB68` and whether it is the
  safe-mode-active flag.
- (Resolved 2026-10-20.) `FUN_1800AB850` versus `FUN_18019A0C0`: both call
  `createConvention` then register, but they are different helper variants distinguished by
  callback signature/arity, not by semantics. `FUN_18019A0C0` is the variant used by
  `FUN_180199D10` for its three hooks; `FUN_1800AB850` is the more common variant. The
  caller-count asymmetry reflects how many registration sites use each signature, not a
  behavioural difference.
- (Unchanged 2026-10-20.) The lazy-thunk family is large and mechanical: 338 registration
  sites imply at least that many thunks, but only 27 callers of `FUN_1801A4300` and 2 of
  `FUN_1801A5190` were counted. No index enumerates the family. UNVERIFIED: the true size of
  the GD surface reached through `GetModuleHandleA(NULL) + rva`; deferred to a future thunk
  index.

## Failure modes recorded

- **`GetModuleHandleA(NULL)` in a decompiler looks like a dead call.** `FUN_18010DDC0` has
  zero meaningful-looking parameters and a body of initialiser boilerplate, so it reads as
  a stub. Its 278 callers are what give it away: a resolver that everyone uses, hidden
  behind a function that looks like plumbing.
- **A hook callback's callee is not necessarily the GD original.** `FUN_1801A4300` is
  Mega Hack code that *resolves and calls* GD code, and it lives in Mega Hack's own image,
  so `call 0x1801a4300` in a disassembly listing reads like a call into the mod. Two
  levels of indirection separate the disassembly from the game.