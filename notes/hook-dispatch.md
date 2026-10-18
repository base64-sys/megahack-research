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

## What this means for the milestones

- **M1, "verified GD offsets"**: three GD function RVAs and one GD field offset are now
  evidence-backed rather than assumed, each from two independent angles. `GJGameLevel+0x148/
  +0x14C/+0x158/+0x198/+0x1B8/+0x2C4/+0x35C` remain unverified; nothing here speaks to them.
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

- `FUN_18019B5F0` — what does the shared gate test? It decides whether three hooks are
  transparent or active, so it is probably a "safe mode" or "level is hackable" predicate.
- `FUN_18019B330` — Mega Hack's post-`resetLevel` work; this is the closest thing to a
  "post-update tracking" chokepoint.
- `FUN_1800AB850` versus `FUN_18019A0C0`: both call `createConvention` then register, but
  they are distinct functions. Whether they differ in hook kind (member vs. free) is not
  established.
- The lazy-thunk family is large and mechanical. Counting them would give the true size of
  the GD surface, but no index exists for it yet.

## Failure modes recorded

- **`GetModuleHandleA(NULL)` in a decompiler looks like a dead call.** `FUN_18010DDC0` has
  zero meaningful-looking parameters and a body of initialiser boilerplate, so it reads as
  a stub. Its 278 callers are what give it away: a resolver that everyone uses, hidden
  behind a function that looks like plumbing.
- **A hook callback's callee is not necessarily the GD original.** `FUN_1801A4300` is
  Mega Hack code that *resolves and calls* GD code, and it lives in Mega Hack's own image,
  so `call 0x1801a4300` in a disassembly listing reads like a call into the mod. Two
  levels of indirection separate the disassembly from the game.