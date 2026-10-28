# Android input-hook parity

- **Question** — which input methods that Mega Hack hooks on Windows can the Android
  build address, and through which channel? This is the first re-application question for
  M5, because input is where the two platforms diverge most.
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Binaries** — `absolllute.megahack.android64.so` (10,721,792 bytes, SHA-256
  `06489f3b…31da`) and `absolllute.megahack.android32.so` (7,591,352 bytes, SHA-256
  `57bbe8d8…b913`), per [provenance.md](provenance.md).
- **Status** — two passes. The first scanned **defined** `_ZN` symbols and reported "0" for
  every method; that measured Mega Hack's own code, not its host surface. The second traced
  the name-resolved registration sites and their detours. The first table is replaced below;
  the detour attribution is a separate section.

## Method

1. Take the 13 distinct `(class, method)` pairs from the Windows input hook sites in
   [input-hooks.md](input-hooks.md), which were read from the registration arguments and
   callback bytes.
2. Dump the dynamic symbol table of both `.so` files with
   `readelf --wide --symbols`. Only `.dynsym` is present (11,240 entries on android64), so
   there is no separate static-symbol surface.
3. Classify each target as **import** when its exact Itanium mangling is a `UND` row, e.g.
   `_ZN12PlayerObject10pushButtonE12PlayerButton`.
4. Scan the complete file bytes for two name forms: the mangled prefix
   `_ZN<len(class)><class><len(method)><method>` and the demangled label
   `Class::method`. A runtime symbol lookup needs one of these strings even when the call
   is not statically linked.
5. **Absence boundary** — a target counts as absent only if it is not in `.dynsym`, its
   mangled prefix does not occur anywhere in either file, and its demangled label does not
   occur anywhere in either file. Both ABIs are checked independently.

## Evidence — 15 Windows input targets on Android

Counts are per file; `mangled` is the `_ZN…` prefix and `demangled` is `Class::method`.

| Windows target (registrar VA) | android64 m/d | android32 m/d | Channel |
| --- | --- | --- | --- |
| `CCEGLViewProtocol::handleTouchesBegin` (`0x18014ED70`) | 0/0 | 0/0 | **absent** |
| `CCMouseDispatcher::dispatchScrollMSG` (`0x18014ED70`) | 0/0 | 0/0 | **absent** |
| `CCKeyboardDispatcher::dispatchKeyboardMSG` (`0x18014ED70`, `0x180190280`) | import | import | import |
| `PlayerObject::pushButton` (`0x1801570E0`, `0x1801812F0`, `0x180183CA0`) | import | import | import |
| `PlayerObject::releaseButton` (`0x1801812F0`, `0x180183CA0`) | import | import | import |
| `SliderTouchLogic::ccTouchMoved` (`0x18015D320`) | 1/1 | 1/1 | name |
| `GJScaleControl::ccTouchMoved` (`0x180163560`) | 1/1 | 1/1 | name |
| `CCTextInputNode::onTextFieldAttachWithIME` (`0x18015D6D0`) | import | import | import |
| `UILayer::handleKeypress` (`0x180173FD0`, `0x180177B00`, `0x180184920`) | import | import | import |
| `PauseLayer::keyDown` (`0x180184920`) | import | import | import |
| `MenuGameLayer::ccTouchBegan` (`0x180189260`) | 1/1 | 1/1 | name |
| `LevelAreaLayer::onClickDoor` (`0x180189A70`) | 1/1 | 1/1 | name |
| `GJBaseGameLayer::handleButton` (`0x180198BF0`, `0x18019CA80`) | import | import | import |
| `EditorOptionsLayer::onButtonRows` (`0x180163F50`) | 1/1 | 1/1 | name |
| `EditorOptionsLayer::onButtonsPerRow` (`0x180163F50`) | 1/1 | 1/1 | name |

Seven targets are direct `UND` imports, six are name-resolved only, and two are absent.
The android32 and android64 results are identical for all fifteen rows.

The name-resolved rows sit in a table that also carries other known hook targets — for
example `_ZN9LevelPage17updateDynamicPageEP11GJGameLevel`,
`_ZN12CreatorLayer13onSecretVaultEPN7cocos2d8CCObjectE` and
`_ZN17RewardUnlockLayer15showCloseButtonEv` — which is consistent with one hook-target
name table rather than incidental strings. Naming alone would be a lead, not proof; the
registration sites traced below turn three of the six name-resolved rows into installed
hooks.

## Findings

1. **The original "0 for everything" table measured the wrong surface.** Of the 2,812
   `_ZN` names in android64's `.dynsym`, 1,964 are defined (Mega Hack's own code) and 848
   are undefined imports. A host method Mega Hack hooks is never a *defined* symbol in the
   mod; it is either an import or a name string.
2. **Two addressing channels exist on Android.** Seven targets appear as exact `UND`
   imports. Six more appear only as a mangled string plus a demangled label, which is what
   a runtime lookup needs. Both encodings occur in both ABIs, and the class-level check
   agrees: `SliderTouchLogic` has no symbol at all, while `GJScaleControl` exports only
   `skewFromValue`/`updateLabelXY` and `MenuGameLayer` only `update`, yet all three methods
   are named in the string table.
3. **Only two Windows input hooks have no Android anchor:**
   `CCEGLViewProtocol::handleTouchesBegin` and `CCMouseDispatcher::dispatchScrollMSG`.
   Neither the mangled nor the demangled form occurs anywhere in either `.so`, and the
   class name `CCEGLViewProtocol` never occurs either. This is consistent with the platform:
   Android has no mouse scroll, and touch is delivered per layer. The Android build instead
   imports `cocos2d::CCLayer::ccTouchesBegan/Ended/Moved/Cancelled`,
   `CCTouchDispatcher::addTargetedDelegate`/`removeDelegate` and
   `CCTouchDelegate::get/setPreviousPriority` — the per-layer touch surface that replaces
   the view-level hook.
4. **Keyboard is still observable on Android.** `dispatchKeyboardMSG` is imported on both
   ABIs, while every `KEYBINDS/*` key is Windows-only ([android-abi.md](android-abi.md)).
   Observing the keyboard dispatcher and shipping a keybind UI are separate decisions, so
   the two facts do not conflict.
5. **Consequence for M5.** Thirteen of the fifteen input targets transfer by name, and six
   of those need no new static anchor. The remaining two are platform-appropriate gaps
   rather than missing evidence. Three of the six name-resolved targets are now shown to be
   installed hooks, not leftover labels; see below.

## The name strings are hook targets, not labels

**Method.** ARM64 disassembly with capstone, driven by the file offsets above:

1. Locate `ADRP`+`ADD` pairs in `.text` (VA = file offset in this image, `.text` at
   `0x431800`) whose sum lands on a target string.
2. Resolve every `BL` through `.plt` (`.plt` at `0x921570`, 5,320 stubs) to the imported
   symbol in `.rela.plt`.
3. Follow the callee chain of the registration site until it reaches a Geode hook import.

**Worked example — `SliderTouchLogic::ccTouchMoved` (`0x4C7F80`):**

| Address | Instruction / effect |
| --- | --- |
| `0x4C7F80`+`0x4C7F84` | `ADRP`+`ADD` → `0x2080C8`, the mangled name; the copy writes 69 bytes and a NUL at `+0x45`, so the extent comes from the stores, not from guessing where the string ends |
| `0x4C7FAC`+`0x4C7FB0` | `ADRP`+`ADD` → `0x1EDA3C`, `"libcocos2dcpp.so"` (16 bytes; built as a libc++ SSO string at `sp+8` with length byte `0x20`) |
| `0x4C7FD0`–`0x4C7FDC` | `x0=mod`, `x1=name`, `x2=lib`; `BL 0x487CCC` — symbol resolver, returns the target in `x0` |
| `0x4C7FE0`–`0x4C8000` | `x1=target`, `x2=0x4C8108` (`ADR`, the detour), `x3=0x213445` (`"SliderTouchLogic::ccTouchMoved"`), `w4=0xBD8`, `w5=1`; `BL 0x4C81D0` |
| `0x4C8218` | `BL 0x921A50` → imported `geode::hook::createConvention(TulipConvention)` |
| `0x4C8244` | `BL 0x4868FC` → `BL 0x48703C`; that helper calls imported `geode::Hook::create` at `0x4870C0` and `geode::Mod::claimHook` at `0x487124` |

The same shape occurs twice more, with the same resolver and the same `0x4C81D0` helper:

| Target | Registration site | Detour (`ADR`) | Display name | Resolver call |
| --- | --- | --- | --- | --- |
| `MenuGameLayer::ccTouchBegan` | `0x533730` | `0x5339D8` | `0x22665D` | `BL 0x487CCC` |
| `LevelAreaLayer::onClickDoor` | `0x534460` | `0x534848` | `0x209FB1` | `BL 0x487CCC` |

The three remaining name-only targets have the same shape. Their helper is not `0x4C81D0`
but two sibling helpers, `0x4CF444` and `0x4D0474`, which were disassembled and both call
imported `geode::hook::createConvention` (`0x921A50`) and then `0x4868FC`, the helper that
reaches `Hook::create` and `claimHook`:

| Target | Registration site | Helper | Detour (`ADR`) | Display name | `w4` | `w5` |
| --- | --- | --- | --- | --- | --- | --- |
| `GJScaleControl::ccTouchMoved` | `0x4CE8F0` | `0x4CF444` | `0x4CEDD0` | `0x20DB75` | `0xBD8` | 1 |
| `EditorOptionsLayer::onButtonRows` | `0x4CFE10` | `0x4D0474` | `0x4D0044` | `0x20DB92` | `0xFFFFF428` (`-0xBD8`) | 1 |
| `EditorOptionsLayer::onButtonsPerRow` | `0x4CFEC8` | `0x4D0474` | `0x4D025C` | `0x20630A` | `0xFFFFF428` (`-0xBD8`) | 1 |

The mangled names those sites resolve are `0x215174`, `0x20297B` and `0x1F574C`; the
registration sites were found by scanning all of `.text` for `ADRP`+`ADD` pairs landing on
those strings. The two Editor sites share one callback: the two `BL 0x4D0474` calls are
guarded by the same enable byte, and the disabled path removes both detours.

**Conclusion.** All six name-only input targets are installed through the normal Geode path:
the build resolves the mangled name from `libcocos2dcpp.so`, passes the detour and the
demangled label to a helper that calls `createConvention`, `Hook::create` and `claimHook`. The
name string is therefore the hook channel, not an unused label.

## Feature attribution of the three traced detours

**Method.** Disassembled the three Android detours with capstone and resolved every `BL`/`B`
through `.plt`; read the matching Windows detours from `recovered/functions/` and the
feature registrations from [hook-registrations.md](hook-registrations.md).

| Android target | Android detour | Windows detour | Shared behaviour |
| --- | --- | --- | --- |
| `SliderTouchLogic::ccTouchMoved` | `0x4C8108` | `0x18015D390` (tag 38, Slider Bypass) | flag at `+0x194`; `CCTouch::getLocation`; `CCNode::convertToNodeSpace`; call a virtual with the y when the flag is 1 and the x otherwise (Android `+0xE8`/`+0xD8`, Windows `+0xE0`/`+0xD0`); then `Slider::updateBar` (import, Android) vs `FUN_1801FF740` (Windows); then one more virtual on the slider (`+0x3E0` vs `+0x368`) |
| `MenuGameLayer::ccTouchBegan` | `0x5339D8` | `0x180189400` (tag 457, Main Menu Gameplay) | null-check the owner, test two state bytes (Android `+0x996`/`+0x993`, Windows `+0x9BE`/`+0x9BB`), call `PlayerObject::pushButton(1)`, set one global byte (Android `0xA420452`, Windows `0x18080C920`), return 1; otherwise return 0 |
| `LevelAreaLayer::onClickDoor` | `0x534848` | `0x180189F90` (tag 449, No Transition) | bare tail call: Android `b 0x923660` = `LevelAreaLayer::onEnterTower`; Windows `jmp 0x18014D3E0` = MH's lazy host-symbol binder for host RVA `0x2C57E0` |

**Conclusion.** The three name-resolved hooks are the Android counterparts of Slider Bypass,
Main Menu Gameplay and No Transition, by target and by detour behaviour. The Windows
structures differ in base offsets and vtable slots, so the claim is feature identity, not
layout identity. For No Transition the two builds take the same shortcut (skip the door
transition and enter the tower), which makes the open `0x220` disagreement in
[host-abi.md](host-abi.md#one-disagreement-left-open) more likely to be `onClickDoor` vs
`onEnterTower` than a naming error.

The Android build does not use the Windows settings table: `android64` contains exactly one
`HACK_*` literal, `HACK_NOCLIP` at `0x211796` (raw-byte search of the whole image), so
feature state on Android is not addressed by `HACK_SLIDER_LIMIT`-style tags.

## The last three name hooks: scale slider and editor steppers

**Method.** Same capstone pass as above, plus a Windows-side RIP-relative reference scan
(`capstone`, x86-64) so that each Android detour could be compared with the Windows detour
that the tag registration installs. The scan computes the target of every `LEA`/`MOV` whose
memory operand uses `RIP`; it is what connects a callback function pointer to the function
that registers it, since the pointer is a displacement, not a literal in `.data`.

**`GJScaleControl::ccTouchMoved` is the scale-slider half of Slider Bypass (tag 38).**

- Windows: `recovered/functions/180163a20_FUN_180163a20.c` is the detour for RVA `0x12A770`,
  installed by `FUN_180163560` together with a second hook on `EditorUI::scaleObjects`
  (RVA `0x121450`). `FUN_180163560` takes one `bool`, installs both hooks when it is true and
  removes both when it is false — the shape of a settings listener.
- Windows: the reference scan finds `0x18016342F: lea rax, [rip+0x12A]` = `0x180163560`
  inside `FUN_180163400`. That function builds a `std::_Func_impl_no_alloc<void(bool)>` whose
  target pointer is `FUN_180163560`, then calls `FUN_1801083f0(local_48, 0x26)` before
  registering the callback with `FUN_1800915b0`. Tag `0x26` is 38, `HACK_SLIDER_LIMIT`
  ([hook-registrations.md](hook-registrations.md) line 68) — the same tag whose action
  `FUN_18015d320` installs `SliderTouchLogic::ccTouchMoved`. Two listeners on one tag explain
  the v9.1.0 changelog line "Slider Bypass now supports scale slider".
- Android: detour `0x4CEDD0` performs the same edit as the Windows detour — touch-id guard
  `[x0+0x1B0] == [x1+0x38]`, the slider at `[x0+0x1A8]`, the label at `[x0+0x1E8]`, the mode
  halfword `[x0+0x1CA] == 2`, `CCNode::convertTouchToNodeSpace` (`0x9228B0`) minus the node x,
  `Slider::getThumb` (`0x9228C0`) positioned through its `+0xB8` virtual, `Slider::getValue`
  (`0x9228D0`), `GJScaleControl::skewFromValue` (`0x9228E0`), rounding to two decimals
  (`×100.0` = `0x4059000000000000`, `frintm`, `÷100.0`), then the label's `+0x10` virtual and
  `GJScaleControl::updateLabelXY` (`0x9228F0`). The Windows detour rounds with
  `floor(v*100)/100` between `FUN_18014e670` and `FUN_180146be0`, and tail-calls the original
  `(*param_1+0x10)` when the touch-id guard fails; Android tail-branches to vtable `+0x378`.

**The two `EditorOptionsLayer` hooks are Toolbox Button Bypass (tag 47).**

- Windows `_hook_sites.tsv` lines 119-120 pair the same two host methods: `onButtonRows`
  (RVA `0x291A00`) → `0x180164020` and `onButtonsPerRow` (RVA `0x291970`) → `0x1801641B0`,
  installer `0x180164340`, callback `0x180163F50`, tag 47
  ([hook-registrations.md](hook-registrations.md) line 77).
- Android detour `0x4D0044` (`onButtonRows`) and `0x4D025C` (`onButtonsPerRow`) are the same
  algorithm on different fields: read the clicked button's tag through the sender's
  `[vtable+0x40]` virtual; if the tag is 1 (the "+" button) add one to the stored count,
  otherwise replace it with `max(1, count-1)` (`csel`/`csinc` pair at `0x4D0094`–`0x4D00B4`
  and `0x4D02AC`–`0x4D02CC`); there is **no upper clamp**, which is the bypass itself. The
  new value is formatted with `"{}"` (`0x1F347D`, two bytes + NUL) and pushed through the
  label's `[vtable+0x390]` virtual (`setString`). `onButtonRows` uses count `+0x444` and label
  `+0x450`; `onButtonsPerRow` uses `+0x440` and `+0x448`.
- The Windows fields for the same two functions are `+0x3F4`/`+0x400` and `+0x3F0`/`+0x3F8`
  ([ui-addons.md](ui-addons.md#evidence--toolbox-button-bypass-2026-10-25)), so all four
  Android fields are exactly `+0x50` from their Windows counterparts. Two unrelated classes
  keeping a constant layout delta is independent support for the pairing.

**Conclusion.** The last three name-resolved Android hooks are the scale-slider listener of
Slider Bypass (tag 38) and the two Toolbox Button Bypass stepper replacements (tag 47). With
this, every name-resolved input hook in the Android build is attributed to a Windows feature.

## Open questions

- The `0xBD8` / `-0xBD8` argument at these sites is a handler-metadata value. Its meaning
  (`priority`? `after`?) is not established, and the sign differs between the Slider site and
  the two Editor sites while the Windows registrations use `0xBD8` for all three. UNVERIFIED:
  what the helper does with `w4`; closing it needs `0x4CF444`/`0x4D0474` traced into
  `Hook::create`'s argument layout.
- UNVERIFIED: that host RVA `0x2C57E0` (the Windows No Transition forward) is
  `LevelAreaLayer::onEnterTower`. Closing this needs a GD 2.2081 symbol anchor; the Android
  import name and the `0x220` offset next to `onClickDoor` are consistent with it but are
  not that anchor.
- UNVERIFIED: whether the tag-38 scale-slider listener (`FUN_180163400`, callback
  `FUN_180163560`) is registered when the tag changes as well as at load. The
  `FUN_1800915b0` call in `FUN_180163400` is not identified; the Android counterpart's
  enable/disable branches are consistent with a listener, but neither build was observed at
  runtime.
- UNVERIFIED: whether the two absent methods exist in the Android GD game binary. The
  absence above is bounded to the Mega Hack `.so` and to its complete file bytes; no
  Android GD image is in `samples/`.
- Count discrepancy, closed 2026-10-28: the two figures measure different things and both
  reproduce. `readelf --wide --symbols` gives 2,812 `_ZN` names in `android64` `.dynsym`
  (1,964 defined, 848 undefined) and 2,806 in `android32`; a raw byte scan for `_ZN...`
  tokens over the complete file gives 3,052 and 3,051 occurrences (2,898 and 2,892
  distinct). The ~240 extra occurrences are the mangled names stored as data - the hook
  target strings above - so [android-abi.md](android-abi.md) quotes the byte count and this
  note quotes the symbol count. Neither is wrong; the earlier "unrecorded query" reading was
  the error.

## What this changes

The M5 note's line that "Android parity must be driven by string keys and behaviour, not by
class/method names" ([android-symbol-parity.md](android-symbol-parity.md)) is too strong for
input: thirteen of fifteen host methods are addressable **by name**, and seven are linked
directly. The useful distinction is not "Windows has names, Android has none" but "Android
has two name channels (imports and runtime strings) and two real gaps (view-level touch
begin, mouse scroll)".
