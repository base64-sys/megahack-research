# Hook-target verification against the host image

- **Question** — every one of the 338 registrations in
  [hook-dispatch.md](hook-dispatch.md) names a target: a hard-coded RVA into Geometry Dash
  (308 rows) or a runtime-resolved name (30 rows). Which of those targets can be confirmed
  from a second, independent angle, and which are still a single lead?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1, image base `0x180000000`
  (`_original.dll`, SHA-256 `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`).
- **Host image used for the second angle** — `C:\Users\Robert_\Documents\GD-Runtime-511\
  GeometryDash.exe`, GD 2.2081, SHA-256
  `FC5A16C292278BC2E8E078FB1D5023C2BD658322DD72712767EA70C2DD9EC6D0`
  (the copy recorded in [runtime-validation.md](runtime-validation.md); the same file the
  release mod loads under in the `runtime511` profile). Read only; never modified.
- **Date** — 2026-10-27.
- **Tools** — `recovered/functions/_hook_sites.tsv`, `_wrappers.tsv`, `_index.tsv`
  (Ghidra-derived, from `scripts/hook_sites.py` / `scripts/resolve_wrappers.py` /
  `scripts/decompile_all.py`); `scripts/pe_rva.py` (`Image.read`, so no RVA→offset
  arithmetic); a `.pdata` reader written for this note. No Ghidra import was needed.

## Why `.pdata` is the right host-side check

Mega Hack's target is `GetModuleHandleA(NULL) + rva`. A hook installed at an address that
is not the entry of a real host function is a silent behaviour change, so the question is
"is this RVA a function entry in GD 2.2081?". On x86-64 every non-leaf MSVC function needs a
`RUNTIME_FUNCTION` record for exception unwinding, and GD's `.pdata` is present and dense:
**14,937 entries** over `.text` (`0x1000`–`0x51306C`). A `BeginAddress` match is therefore a
precise, mechanical test, not a heuristic: no pattern matching, no naming, no decompiler.

It is a genuinely independent angle from the current evidence, because the current RVAs come
from *Mega Hack's* instruction stream (a registration argument and, for some rows, a thunk
addend), while `.pdata` comes from *Geometry Dash's* own image.

## Result: 183 of 188 distinct RVAs are exact function entries

The 308 RVA rows collapse to **188 distinct RVA values** (every one of the 215 methods maps
to exactly one target, and 25 registrations of `PlayLayer::resetLevel` share one RVA).

| Check | Count |
| --- | ---: |
| Distinct hook RVAs | 188 |
| Exact `.pdata` `BeginAddress` match | **183** |
| Not a `BeginAddress` and not interior to any recorded function | 5 |
| Also re-derived by a thunk addend (`_wrappers.tsv`) | 145 |
| `.pdata` only (no thunk addend) | 38 |
| Thunk addend only (no `.pdata`) | 3 |
| Neither angle (single lead) | **2** |

Two angles now hold for 145 RVAs; one angle holds for 41; two RVAs have only the
registration argument.

### The five RVAs with no `.pdata` record

| RVA | Method(s) | Thunk addend? | First bytes at the RVA |
| --- | --- | --- | --- |
| `0xC77D0` | `SongSelectNode::audioNext` | no | `8b 81 58 01 00 00` (`mov eax,[rcx+0x158]`) |
| `0xC7810` | `SongSelectNode::audioPrevious` | no | `8b 81 58 01 00 00` |
| `0x23AD00` | `GJBaseGameLayer::loadLevelSettings` | yes | `48 8b 81 b0 0d 00 00` |
| `0x38D480` | `PlayerObject::runBallRotation2` | yes | `80 b9 e4 09 00 00 00` |
| `0x3AB9A0` | `PlayLayer::updateTimeWarp` | yes | `80 b9 70 35 00 00 00` |

All five are backed bytes inside `.text`, and none lies inside another function's
`RUNTIME_FUNCTION` range, so they are not interior labels. The most economical reading is
that they are `this`-field loads at the entry of **leaf functions that emit no unwind
record**: MSVC omits `.pdata` for a function with no stack frame and no non-volatile
register saves, and a C++ member function may begin directly with a field load. For the
three with a thunk addend the thunk independently re-derives the same constant, so the value
is corroborated even though the boundary is not.

UNVERIFIED: that these five are function *entries* rather than interior addresses. Closing
it needs a host-side function-boundary pass over GD 2.2081 (Ghidra or Rizin `af` over
`0xC7700`–`0xC7900`, `0x23AC00`–`0x23AE00`, `0x38D400`–`0x38D500`, `0x3AB900`–`0x3ABA00`)
that reports the recovered start for each.

UNVERIFIED (weakest pair): `0xC77D0` / `0xC7810` have no thunk addend either. They are
single-angle rows. A registration argument that was mistyped would look exactly like this,
so they must not be quoted as verified host addresses until a second angle exists.

## What the same probe says about the Mega Hack side

The Ghidra-derived local indexes already answer most of the mod-side question exactly:

| Set | Distinct | Exact procedure entry in `_index.tsv` |
| --- | ---: | ---: |
| Registrar functions | 164 | **164** |
| Registration helper functions | 143 | **143** |
| Wrapper thunk functions | 277 | **277** |
| Callbacks | 338 | 276 |
| Callbacks with `callback_kind=label` | 43 | 0 (Ghidra labels, not procedures) |
| Callbacks with `callback_kind=data` | 19 | 0 (data slots) |

`_wrappers.tsv` is internally consistent: 277 rows, 0 rows where
`hook_target_va != hook_target_rva + 0x180000000`, 180 distinct resolvers.

So the mod-side mapping is **276 exact, 62 open**. The 62 open callbacks are the ones
`hook_sites.py` found as `LAB_`/`DAT_` operands; they are registered as callbacks but Ghidra
did not split them into procedures, so their owning function and their call edges are
unknown. That is the only mod-side gap a Ghidra pass has to close
(`inspect_address_context` per address), not a 338-row problem.

## Correction: `_hook_sites.tsv` carries no per-row verification flag

[ROADMAP.md](../docs/ROADMAP.md) (M1 residuals) says "Hook-target RVAs still flagged
UNVERIFIED per row in `_hook_sites.tsv`". The current file has eight columns —
`registrar_va, gd_method, gd_rva, target_kind, callback, callback_kind, helper, order` — and
no verification column; `scripts/hook_sites.py` does not emit one. The per-row flag was lost
when the index was regenerated. Rather than add a hand-maintained flag, the two angle flags
above (`pdata`, `thunk`) are reproducible from the tracked indexes on every regeneration and
should be joined, not stored.

`notes/runtime-validation.md` said `_hook_sites.tsv` has "317 RVA sites" and
`hook-dispatch.md` said "(317 sites)" in one paragraph; the file has **308 RVA + 30 name =
338**. Both were corrected in place on 2026-10-27. 317 is not a count this index has ever
supported: 338 − 30 = 308.

## Correction: the host-method string inventory is a subset, not the universe

`_host_methods.tsv` has 214 rows and is derived from `_strings.tsv`'s `host-method`
category, which is a **classified subset**: 4,175 rows out of the 9,957 defined strings
Ghidra reports for this DLL (`binary_overview`, REA Evidence
`ev_975873ce4252956dcc1844c4aa37ee76fec8f66f88fb54fc238435a9d528aa99`).

Ten of the 215 hooked methods have **no row at all**, even though the name literal is
present in Mega Hack's own code:

| Method | Target kind | Literal appears in |
| --- | --- | --- |
| `BoomListView::init` | rva | `1801863b0` |
| `CCCircleWave::draw` | rva | `18019acb0` |
| `GJBaseGameLayer::handleButton` | rva | `180198bf0`, `18019ca80` |
| `PauseLayer::keyDown` | rva | `180184920` |
| `PlayLayer::getCurrentPercent` | rva | `180165040` |
| `PlayerObject::playerDestroyed` | rva | `180173d00` |
| `PlayerObject::releaseButton` | rva | `1801812f0`, `180183ca0` |
| `PlayerObject::switchedToMode` | rva | `180165890` |
| `cocos2d::CCDirector::drawScene` | **name** | `180156d00`, `180156ed0` |
| `cocos2d::CCMotionStreak::initWithFade` | **name** | `1801732c0`, `1801734c0` |

The two `name` rows matter beyond bookkeeping: they are among the 30 name-resolved sites
that [runtime-validation.md](runtime-validation.md) has no resolver strategy for. A resolver
built from `_host_methods.tsv` would silently miss them.

UNVERIFIED: why `classify_strings.py` drops these ten (likely no VA anchor / SSO form), and
whether more hooked names are missing for the same reason. Closing it needs a re-derivation
of the `host-method` category from the raw literal scan rather than from the classified set.

## Ghidra pass (2026-10-27): what a live import adds

One import of `_original.dll` (Ghidra 12.1.2 via REA 6.7.0; first query returns in **363 s**)
produced two whole-program inventories, so no per-address query is needed for identity:

| Call | Evidence ID | Size |
| --- | --- | --- |
| `list_procedures` | `ev_f29fa066d83c83f185459438b2b6c1ff0b7f9da246013bdec1b518b83befdf97` | 20,455 procedures |
| `list_strings` | `ev_a455a3e3c1fe6a6d6611008617ffa806727be09275983a7c1889cd74d0b6b91b` | 9,957 defined strings (9,936 US-ASCII, 21 UTF-16) |

The 20,455 divide into **18,825 local non-thunk + 671 thunks + 959 external** (the externals
are `GEODE.DLL::…` import thunks). `_index.tsv` holds exactly the **18,825** local non-thunk
procedures, so the long-standing "20,455 vs 18,825" discrepancy is entirely the thunk set:
`decompile_all.py` never wrote a `.c` for a thunk. Nothing else is missing.

Against those live procedures:

| Set | Distinct | Exact procedure entry | Not a procedure |
| --- | ---: | ---: | ---: |
| Registrars | 164 | 164 | 0 |
| Helpers | 143 | 143 | 0 |
| Repo "wrapper thunks" | 277 | 277 (none is a Ghidra thunk) | 0 |
| Callbacks | 338 | 276 | 62 |

Artifacts kept for reuse, both gitignored: `recovered/rea-bundle-20261027.json` (7.9 MB,
the two Evidence records) and `recovered/ghidra-snapshot-20261027.json` (10.2 MB, 124
primitive entries, 402 evidence records). The snapshot re-opens the same analysis through
`open_binary(snapshot_path=…)` without a re-import; it must match the binary digest,
provider and analysis profile exactly, and any metadata edit would invalidate it.

## The 62 callbacks that neither engine places in a function

Ghidra was asked about all 62 (`inspect_address_context`): **every one** returns
`procedure.state="available"`, `found=false`, `reason="not_in_procedure"`. The `.pdata`
reader independently agrees: all 62 are outside every `RUNTIME_FUNCTION` range of the DLL
(19,689 entries). Two engines, one answer — they are addresses that were never promoted to
function boundaries, which is why `hook_sites.py` saw them as `LAB_` (43) and `DAT_` (19).

Decoding each address with capstone until the first `ret`/`jmp`/`int3` gives their shape:

| Shape | Count | What the callback is |
| --- | ---: | --- |
| `jmp <wrapper thunk>` | 21 | call the host original and nothing else |
| bare `ret` | 19 | do nothing at all |
| body ending in `ret`, no call and no jmp | 9 | read/write Mega Hack state, then return |
| `jmp <other Mega Hack function>` | 8 | forward into Mega Hack code |
| `jmp <register/memory>` | 2 | indirect forward |
| no terminator within 12 instructions | 1 | `PlayLayer::updateAttempts`, `0x180158F00` |

Two examples, both one instruction away from being invisible to a call-graph tool:
`0x18015F900` (`EditorUI::constrainGameLayerPosition`) is a single `c3`; `0x18010BEE0`
(`PlayLayer::init`) is `mov dword ptr [rip+0x6F5A02],0` then `jmp 0x1801A4F30`, which is a
`_wrappers.tsv` thunk.

**Inferred, not observed:** the 30 callbacks that contain no call and no jmp (19 bare `ret`
+ 9 state-only bodies + 2 `xor eax,eax; ret`) never invoke the host original, so under the
`TulipConvention` established in [hook-dispatch.md](hook-dispatch.md) the host method body
does not execute while the hook is installed. That reading rests on the convention being
"the callback calls the original if it wants it", which the release's own wrappers
demonstrate; it is not a runtime observation. UNVERIFIED: a trace showing a suppressed
host method (e.g. `PlayerObject::spawnPortalCircle`) not running.

## Original-call edges: 242 of 276 confirmed by a direct callee

Ghidra's `procedure_callees` was read for each of the 276 callbacks that *are* procedures:

- **242** have at least one direct callee that is a `_wrappers.tsv` thunk — a positive,
  machine-checked "this callback calls the host original" edge.
- **34** have none: 20 of those are the name-resolved hooks, where no RVA thunk exists by
  construction, and **14 are RVA-resolved** — `CCAnimatedSprite::runAnimation`,
  `CCAnimatedSprite::tweenToAnimation`, `CCCircleWave::draw`,
  `EditorOptionsLayer::onButtonRows`, `EditorOptionsLayer::onButtonsPerRow`,
  `EndLevelLayer::showLayer`, `GJBaseGameLayer::init`, `HardStreak::updateStroke`,
  `OptionsLayer::onSecretVault`, `PlayLayer::destroyPlayer`, `PlayLayer::storeCheckpoint`,
  `PlayerObject::updateDashAnimation`, `SecretLayer2::onDoor`, `SecretRewardsLayer::onShop`.
- The four doubly-confirmed wrappers reproduce exactly, which is the control on this data:
  `0x180199E80→0x1801A4300`, `0x180199DF0→0x1801462C0`, `0x180199E30→0x1801A5190`,
  `0x180174DE0→0x1801A5260`.
- Every distinct callee that falls inside the thunk region `0x1801A3000`–`0x1801A7FFF`
  is already a `_wrappers.tsv` row (39 of 39), so the index is not simply missing thunks
  that callbacks call directly.

The 14 are therefore an **open item, not a claim**: their original call may be a
memory-indirect tail jump (which produces no callee edge) or may genuinely be absent. This
is the one part of "does the callback forward?" that a call graph alone cannot settle.

## Strings: 214 of 214 exact, and ten names outside the inventory

- Every one of the 214 `_host_methods.tsv` rows resolves to a defined Ghidra string whose
  text matches the method name **exactly** (0 text mismatches, 0 absent VAs).
- The ten hooked methods with no row (above) have **zero hits among all 9,957 defined
  strings**. The literal is in the code — the decompiled registration call shows it — but
  Ghidra did not define a string object at it. `list_strings` documents exactly this limit:
  "Only Ghidra-defined string Data is observed; packed or undefined string bytes may be
  absent."
- Consequence for the runtime work: a resolver built from `_host_methods.tsv` would resolve
  the 214 and silently fail on the other ten, **two of which are name-resolved hooks**
  (`cocos2d::CCDirector::drawScene`, `cocos2d::CCMotionStreak::initWithFade`). The index is
  a verified subset, not the universe.

## Open questions

- Are the five non-`.pdata` RVAs function entries? See the boundary-pass above.
- What do the 14 RVA-resolved callbacks without a thunk callee actually do — memory-indirect
  tail jump, or no forward at all? Closing it needs per-callback disassembly, which is
  hook-logic work rather than inventory mapping.
- Do any of the 30 no-forward callbacks have a *second* registration for the same method
  that does forward? The registration table allows it; the site-to-method join has to be
  consulted per method.
- Do the 62 label/data callbacks belong to functions already in `_index.tsv`, or to
  functions Ghidra never split? `inspect_address_context` answers this in one Ghidra session.
- Which of the 1,630 Ghidra procedures without a recovered `.c` matter here? None of the
  164 registrars, 143 helpers or 277 thunks is among them, so the gap is currently cosmetic
  for this question — but it is not zero.
- Does hooking an unwind-less leaf function change Geode's trampoline requirements? Not
  investigated; it would matter for the reconstruction's hook installer, not for this map.
