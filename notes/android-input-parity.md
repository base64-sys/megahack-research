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

**Conclusion.** Three of the six name-only input targets are installed through the normal
Geode path: the build resolves the mangled name from `libcocos2dcpp.so`, passes the demo
detour and the demangled label to a helper that calls `createConvention`, `Hook::create` and
`claimHook`. The name string is therefore the hook channel, not an unused label. The other
three (`GJScaleControl::ccTouchMoved`, `EditorOptionsLayer::onButtonRows`,
`EditorOptionsLayer::onButtonsPerRow`) have the same string shape but were not traced this
pass. UNVERIFIED: their registration sites and detours.

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

## Open questions

- UNVERIFIED: the three untraced name-resolved targets above
  (`GJScaleControl::ccTouchMoved`, `EditorOptionsLayer::onButtonRows`,
  `EditorOptionsLayer::onButtonsPerRow`); the next question is which feature callback each
  detour implements, not whether the string is used.
- The `0xBD8` argument at the SliderTouchLogic site is a handler-metadata value. Its meaning
  (`priority`? `after`?) is not established; the Windows equivalent is the `-32` argument in
  the `Hook` registration, but the units need not match.
- UNVERIFIED: that host RVA `0x2C57E0` (the Windows No Transition forward) is
  `LevelAreaLayer::onEnterTower`. Closing this needs a GD 2.2081 symbol anchor; the Android
  import name and the `0x220` offset next to `onClickDoor` are consistent with it but are
  not that anchor.
- `SliderTouchLogic::ccTouchMoved` is now attributed to Slider Bypass on both platforms.
  `GJScaleControl::ccTouchMoved` is still unattributed on Android; a Toolbox Button Bypass
  binding would need its registration site, not the name alone.
- UNVERIFIED: whether the two absent methods exist in the Android GD game binary. The
  absence above is bounded to the Mega Hack `.so` and to its complete file bytes; no
  Android GD image is in `samples/`.
- Count discrepancy: android64 has 2,812 `_ZN` names in `.dynsym` by
  `readelf --wide --symbols` here, while [android-abi.md](android-abi.md) records 3,052.
  The lower figure reproduces on both ABIs (2,812/2,806); the 3,052 was produced by a
  different query that is not recorded. The ABI conclusion does not depend on the count.
  UNVERIFIED: the original query.

## What this changes

The M5 note's line that "Android parity must be driven by string keys and behaviour, not by
class/method names" ([android-symbol-parity.md](android-symbol-parity.md)) is too strong for
input: thirteen of fifteen host methods are addressable **by name**, and seven are linked
directly. The useful distinction is not "Windows has names, Android has none" but "Android
has two name channels (imports and runtime strings) and two real gaps (view-level touch
begin, mouse scroll)".
