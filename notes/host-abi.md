# Host ABI surface: how Mega Hack reaches Geometry Dash

- **Question** — what does Mega Hack statically link against, and how does it reach
  Geometry Dash's gameplay classes?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. `tools/pe_imports.py` / `pefile` `DIRECTORY_ENTRY_IMPORT` — 1,010 imports across 30 DLLs,
   with preferred-base IAT addresses. 27 of these are imported by ordinal (all WS2_32.dll);
   pefile resolves them to names via the standard ordinal table, so they appear as named
   imports in the tooling. Raw-bytes reading: 27 ordinal, 0 unnamed. No GD gameplay classes
   are among them.
2. 646 of those are MSVC-mangled C++ symbols. Demangled with `undname.exe` (VS 2022
   BuildTools 14.44.35207) in **file mode** — one signature per input line, in input order.
   The output count is asserted equal to the input count before anything is parsed; a
   mismatch aborts rather than guesses. PowerShell command-line quoting corrupts `$` in
   mangled names, so symbols are written to a file and passed as a path.
3. Second angle: raw byte search of the whole image for GD class names, and
   `tools/pe_xrefs.py` plus `tools/pe_disasm.py` on the one result.

## Headline finding: the host ABI is two-layered

**No Geometry Dash gameplay class is statically imported.** Zero. Verified by searching all
1,010 demangled imports for ten GD class markers:

| Marker | Imports |
| ------ | ------- |
| `GJGameLevel` | 0 |
| `PlayLayer` | 0 |
| `GamePlayLayer` | 0 |
| `LevelEditorLayer` | 0 |
| `GJEffectManager` | 0 |
| `GJUIWindow` | 0 |
| `GJLayer` | 0 |
| `GameLayer` | 0 |
| `GJSprite` | 0 |
| `GJBase` | 0 |
| `FLAlertLayer` | 1 — but from `Geode.dll`, not the game |

Yet the GD classes are plainly present, as **name strings resolved at runtime**:

| Marker | ASCII occurrences | UTF-16 |
| ------ | ----------------- | ------ |
| `GJGameLevel` | 4 | 0 |
| `PlayLayer` | 29 | 0 |
| `LevelEditorLayer` | 7 | 0 |
| `CCNode` | 170 | 0 |
| `cocos2d` | 401 | 0 |

This is expected and not an anomaly: GD's classes live in `GeometryDash.exe`, not in a
library, so a DLL cannot import them. Mega Hack resolves them through Geode — which is
precisely what the declared dependency `geode.node-ids >= v1.22.0` is for, and why the
import list contains `geode::Mod::hook(void*, void*, std::string, tulip::hook::HandlerMetadata, tulip::hook::HookMetadata)`.

**Consequence for the project:** the import table tells you almost nothing about how Mega
Hack interacts with gameplay. Every one of the 160+ features reaches into GD through this
runtime resolution path, so *that mechanism is the highest-value target in M2* — not the
feature modules.

## Evidence — imports by DLL, classified

| Count | DLL | Class |
| ----: | --- | ----- |
| 470 | `libcocos2d.dll` | GD / cocos2d engine |
| 118 | `MSVCP140.dll` | C++ runtime |
| 91 | `KERNEL32.dll` | Win32 kernel |
| 49 | `Geode.dll` | Geode loader |
| 31 | `api-ms-win-crt-stdio-l1-1-0.dll` | C runtime |
| 27 | `glew32.dll` | OpenGL loader |
| 27 | `WS2_32.dll` | Winsock |
| 22 | `VCRUNTIME140.dll` | C runtime |
| 21 | `USER32.dll` | Win32 user |
| 21 | `ADVAPI32.dll` | Win32 registry/security |
| 20 | `OPENGL32.dll` | OpenGL |
| 19 | `fmod.dll` | FMOD audio |
| 18 | `api-ms-win-crt-runtime-l1-1-0.dll` | C runtime |
| 18 | `api-ms-win-crt-math-l1-1-0.dll` | C runtime |
| 15 | `api-ms-win-crt-string-l1-1-0.dll` | C runtime |
| 10 | `CRYPT32.dll` | Win32 crypto |
| 6 | `api-ms-win-crt-convert-l1-1-0.dll` | C runtime |
| 5 | `api-ms-win-crt-heap-l1-1-0.dll` | C runtime |
| 3 | `libExtensions.dll` | cocos2d extensions |
| 3 | `SHELL32.dll` | Win32 shell |
| 3 each | `api-ms-win-crt-{time,filesystem}` | C runtime |
| 2 each | `api-ms-win-crt-{locale,utility}` | C runtime |
| 1 each | `IPHLPAPI.DLL`, `ole32.dll`, `COMDLG32.dll`, `dwmapi.dll`, `VCRUNTIME140_1.dll`, `api-ms-win-crt-environment-l1-1-0.dll` | misc |

Namespace roots across all 1,010 demangled imports: `cocos2d` 478, flat/unmangled 372,
`std` 103, `geode` 37, `FMOD` 19, `arc` 1. No GD-specific root appears.

## Evidence — the cocos2d engine layer

93 distinct owning classes across 470 imports. The weighting shows where the work is:

| Calls | Class | Calls | Class |
| ----: | ----- | ----: | ----- |
| 116 | `CCNode` | 8 | `CCDirector` |
| 59 | `CCSprite` | 7 | `CCArray` |
| 37 | `CCLayer` | 7 | `CCDictionary` |
| 30 | `CCLabelBMFont` | 6 | `CCDrawNode` |
| 14 | `CCSpriteBatchNode` | 5 | `CCTouchDispatcher` |
| 13 | `CCLayerRGBA` | 4 | `CCClippingNode` |
| 13 | `CCNodeRGBA` | 4 | `CCEGLView` |
| 12 | `CCMenu` | 3 | `CCMouseDispatcher` |
| 12 | `CCLayerColor` | 3 | `CCTextureCache` |
| 10 | `CCMenuItem` | 3 | `CCSpriteFrameCache` |
| 9 | `CCObject` | | |

`CCLabelBMFont` at 30 calls is notable: it is the bitmap-font label Mega Hack's menu text
uses, so the UI layer is built on `CCLabelBMFont` + `CCMenu`/`CCMenuItem` rather than
`CCLabelTTF` (2 calls).

The remaining classes are almost entirely actions and effects — `CCScaleTo`, `CCFadeIn`,
`CCFadeTo`, `CCMoveBy`, `CCSequence`, `CCEaseInOut`, `CCEaseOut`, `CCEaseBounceOut`,
`CCEaseExponentialInOut`, `CCEaseBackOut`, `CCEaseSineOut`, `CCHide`, `CCDelayTime`,
`CCCallFunc`, `CCCallFuncO`. So Mega Hack animates UI transitions through cocos2d actions
rather than hand-rolling a frame loop.

## Evidence — the Geode loader layer (all 49)

Fully named. Grouped by what they imply.

**Mod lifecycle and settings**

| IAT | Signature |
| --- | --------- |
| `0x180642D50` | `geode::Loader::get()` |
| `0x180642E20` | `geode::Loader::takeNextMod()` *(protected)* |
| `0x180642D68` | `geode::Loader::getLoadedMod(std::string_view<char>) const` |
| `0x180642DD8` | `geode::Loader::queueInMainThread(std::move_only_function<void()>&&)` |
| `0x180642D80` | `geode::Mod::getSetting(std::string_view<char>) const -> shared_ptr<geode::SettingV3>` |
| `0x180642DA8` | `geode::Mod::isInternal() const` |

**Hooking**

| IAT | Signature |
| --- | --------- |
| `0x180642D98` | `geode::Mod::hook(void*, void*, std::string, tulip::hook::HandlerMetadata, tulip::hook::HookMetadata) -> Result<geode::Hook*, std::string>` |
| `0x180642DC8` | `geode::Mod::patch(void*, std::vector<unsigned char>) -> Result<geode::Patch*, std::string>` |
| `0x180642D30` | `geode::Mod::disownPatch(geode::Patch*) -> Result<void, std::string>` |
| `0x180642D38` | `geode::Hook::enable() -> Result<void, std::string>` |
| `0x180642D28` | `geode::Hook::disable() -> Result<void, std::string>` |
| `0x180642DA0` | `geode::Hook::isEnabled() const` |
| `0x180642D10` | `geode::hook::createConvention(tulip::hook::TulipConvention) -> shared_ptr<tulip::hook::CallingConvention>` |
| `0x180642D20` | `geode::hook::createWrapper(void*, tulip::hook::WrapperMetadata const&) -> Result<void*, std::string>` |

**Node lifetime** — the `DestructorLock` set plus matching `CCNode` accessors:

| IAT | Signature |
| --- | --------- |
| `0x180642CF8` | `geode::DestructorLock::addLock(cocos2d::CCNode*)` |
| `0x180642DB0` | `geode::DestructorLock::isLocked(cocos2d::CCNode*)` |
| `0x180642DB8` | `geode::DestructorLock::isLocked(void*)` |
| `0x180642DE0` | `geode::DestructorLock::removeLock(void*)` |
| `0x180642D88` | `cocos2d::CCNode::getUserObject(std::string_view<char>)` |
| `0x180642E08` | `cocos2d::CCNode::setUserObject(std::string, cocos2d::CCObject*)` |
| `0x180642E00` | `cocos2d::CCNode::setUserFlag(std::string, bool)` |
| `0x180642DF8` | `cocos2d::CCNode::setID(std::string)` |
| `0x180642D60` | `cocos2d::CCNode::getChildByID(std::string_view<char>)` |
| `0x180642E30` | `cocos2d::CCNode::updateLayout(bool)` |

**Events**

| IAT | Signature |
| --- | --------- |
| `0x180642D40` | `geode::comm::EventCenter::get()` |
| `0x180642D48` | `geode::comm::EventCenterThreadLocal::get()` |
| `0x180642DF0` | `geode::comm::EventCenterThreadLocal::send(BaseFilter const*, move_only_function<bool(OpaquePortBase*)>, move_only_function<OpaquePortBase*(OpaquePortBase*)>)` |

**Paths, logging, UI**

| IAT | Signature |
| --- | --------- |
| `0x180642D70` | `geode::dirs::getModRuntimeDir() -> std::filesystem::path` |
| `0x180642D78` | `geode::dirs::getModsSaveDir() -> std::filesystem::path` |
| `0x180642DD0` | `geode::utils::string::pathToString(std::filesystem::path const&)` |
| `0x180642E38` | `geode::log::vlogImpl(geode::Severity, geode::Mod*, fmt::v12::basic_string_view<char>, fmt::v12::basic_format_args<fmt::v12::context>)` |
| `0x180642D18` | `geode::createQuickPopup(char const*, std::string, char const*, char const*, move_only_function<void(FLAlertLayer*, bool)>, bool, bool) -> FLAlertLayer*` |
| `0x180642DE8` | `geode::async::runtime() -> arc::Runtime&` |

**Settings value access**

| IAT | Signature |
| --- | --------- |
| `0x180642D90` | `geode::SettingBaseValueV3<bool,bool>::getValue() const` |
| `0x180642E10` | `geode::SettingBaseValueV3<bool,bool>::setValue(bool)` |

Note six `cocos2d::CCNode` methods are imported from **Geode.dll**, not from
`libcocos2d.dll`: `getUserObject`, `setUserObject`, `setUserFlag`, `setID`, `getChildByID`,
`updateLayout`. Geode ships its own additions to `CCNode`. Any reconstruction must link
those against `Geode.dll`, not the cocos2d import library.

## Evidence — the runtime resolution path

`tools/pe_xrefs.py absolllute.hackmega_original.dll GJGameLevel` finds four occurrences. One
is a member-name string, three are mangled type names.

| VA | Section | Content |
| -- | ------- | ------- |
| `0x18057AEFC` | `.rdata` | `GJGameLevel::savePercentage` |
| `0x1807F7444` | `.data` | `GJGameLevel@@` |
| `0x1807F7CEE` | `.data` | `GJGameLevel@@_N2@Z@X$$V@std@@` |
| `0x1807F7D67` | `.data` | `GJGameLevel@@_N2@Z@` |

The single code reference to the member-name string is at `0x180199D46`:

```
0000000180199d2f  mov  dword ptr [rsp + 0x28], 1
0000000180199d37  mov  dword ptr [rsp + 0x20], 0xffffffe0
0000000180199d3f  lea  r8, [rip + 0xaa]              ; -> 0x180199de9
0000000180199d46  lea  r9, [rip + 0x3e11af]           ; -> 0x18057aefc "GJGameLevel::savePercentage"
0000000180199d4d  mov  rcx, rsi
0000000180199d50  mov  rdx, rax
0000000180199d53  call 0x18019a0c0
```

The same shape repeats immediately afterwards — `mov edx, 0x3a7a80` … `lea r9, [rip+…]` …
`call 0x1800ab850`, then `mov edx, 0x3b8eb0` — so this is a **repeated registration
sequence**, one per hooked member, not a one-off.

Interpretation: `r9` is the member name, `r8` is a second string, `0xffffffe0` is a
small-integer enum argument, and the callee performs the Geode member-hook registration.
`mov edx, 0x16c8b0`-style immediates preceding each block look like resolved member addresses
or handle indices.

UNVERIFIED: the exact meaning of each argument, and which parameter is the hook target.
Confirming means decompiling `0x18019A0C0` and `0x1800AB850` with
`tools/ghidra_scripts/DumpDecompileFile.java` and tracing the callee to a Geode import.
The alternative reading is that these are string-construction calls rather than hook
registrations.

The three `.data` strings are MSVC-mangled *type* names, not member names:
`GJGameLevel@@_N2@Z@` is the `type_info::name()` payload for a class with two
back-references. Their presence in writable `.data` (rather than `.rdata`) and their
neighbourhood — `...GameLayer@@PEAV` immediately precedes two of them — suggests Mega Hack
emits its own `type_info` objects for GD classes, which would be required for any
`dynamic_cast` or `typeid` against a GD pointer.

UNVERIFIED: that these are RTTI `TypeDescriptor` payloads rather than exception-handling
type-match strings. Confirming means checking whether the 16 bytes before each string hold
the two standard `TypeDescriptor` pointers (`pVFTable`, `spare`) — at `0x1807F7D67` they read
`56 50 6c 61 79 4c 61 79 65 72 40 40 50 45 41 56`, which is ASCII `VPlayLayer@@PEAV`, i.e.
*not* pointers. That contradicts the RTTI reading and leaves both interpretations open.

## How Mega Hack calls host functions it does not import

The import table has zero GD gameplay symbols, yet the code calls GD methods — so the call
path itself is worth recording. It is a **lazy binder in Mega Hack's own image**, and the
stub bodies are the evidence (read 2026-10-25 from No Transition's menu detours, which tail
into them):

```
0x18014CF94  mov   eax, [rip+...]         ; cached generation counter
0x18014CF9A  mov   r8d, [rip+...]         ; per-thread slot index
0x18014CFA1  mov   r9, qword gs:[0x58]    ; thread-local storage block
0x18014CFAA  mov   r8, [r9+r8*8]          ; per-thread table
0x18014CFAE  cmp   eax, [r8+0x10]         ; already resolved?
0x18014CFC5  jmp   rax                    ; fast path: cached pointer
0x18014CFD7  call  0x1803A5140            ; slow path: resolver
```

The slow path gets a module base from `0x18010DDC0`, adds a fixed offset
(`lea rcx,[rax+0x2C57E0]` at `0x18014D444`), calls `0x18014D230`, and caches the result.
The cache lives in `.data`'s zero-fill tail, which reads as zero in the file — consistent
with first-call population.

This is **not** a PE delay-import table: data directory 13 is empty (`rva=0 size=0`), while
directory 12 (IAT) is `0x642CB8` size `0x2080`. The stub family is large: `mov r9, gs:[0x58]`
occurs **81** times and `mov r8, gs:[0x58]` **110** times in the image.

### The base is the game module, and the offset is the host RVA

`0x18010DDC0` is a cached `GetModuleHandleA(NULL)`: it takes no argument, loads
`qword [rip+0x53515A]` → import pointer `0x180642F68` with `ecx=0`, stores the result in
`.data`, and returns it. Two independent readings agree on the import: the Ghidra symbol
table names that slot `PTR_GetModuleHandleA_180642f68`, and the slot's bytes in the file are
`60 60 64 00 00 00 00 00`, i.e. an import-table entry at RVA `0x646060` rather than code.

Therefore each stub computes `game_module_base + immediate`, which makes **the immediate the
host function's RVA**. Scanning `.text` for the `call 0x18010DDC0` / `lea rcx,[rax+imm32]`
pair (`scripts/lazy_stubs.py`) finds **277 stubs over 276 distinct host RVAs** — one RVA has
two stubs.

That converts the stubs from anonymous trampolines into named targets. Joining the 276 RVAs
against the named hook-site table names **148** of them without any runtime observation, and
the names are the expected GD methods:

| Host RVA | Method |
| ---: | --- |
| `0x03C020` | `BoomListView::init` |
| `0x0508F0` | `CCTextInputNode::onTextFieldAttachWithIME` |
| `0x0AB610` | `CustomListView::loadCell` |
| `0x0DF390` | `EditorUI::init` |
| `0x4A5F30` | `EffectGameObject::triggerObject` |

This is also an independent check on the name-resolved hook targets recorded elsewhere: the
same method arrives at the same RVA from two unrelated directions — a `mov edx,<rva>` in a
callback and the immediate in a generated stub. `LevelAreaInnerLayer::init` agrees exactly
(`0x2C64E0`).

Consequence for mapping: a detour that tail-jumps into one of these stubs shows the
arguments it passes to the original, not the host address. The stub's immediate supplies the
address; the arguments still need reading separately.

### One disagreement, left open

`LevelAreaLayer::onClickDoor` is name-resolved to RVA `0x2C55C0` in No Transition's callback,
but the stub that No Transition's detour jumps into adds `0x2C57E0` — `0x220` higher, and
`0x2C57E0` is not named by any hook site. Either the named `0x2C55C0` is a wrapper that
reaches `0x2C57E0`, or the two are different functions. Both readings are consistent with the
bytes; GD's binary would settle it and is not part of this baseline. **UNVERIFIED.**

UNVERIFIED: whether the resolver (`0x18014D230` and its siblings, one per stub) is Mega Hack's
own or a Geode-provided helper compiled into the same image, and what its second argument is.

### Recovering the RVA from a symbol-only hook

The previous subsection said the friendly name is the only identifier for a stub target. A pass
over all 125 registrations on 2026-10-25 shows that is stronger than it needs to be: **every
hook that resolves by mangled symbol also has an RVA recorded against its method name**, and a
large fraction have a lazy stub as well.

| Measure | Count |
| --- | ---: |
| Registrations | 125 |
| Tags with at least one symbol-only hook | 120 |
| Symbol-only hook rows | 201 |
| Rows whose method name is in the hook-site table | 185 |
| Rows that additionally have a lazy stub immediate | 148 |

Two caveats, both load-bearing:

1. The name→RVA join is a **cross-reference, not an independent observation**. The hook-site
   table was produced by the same research pass that recorded these symbol hooks, so agreement
   there only proves consistency. Independence comes from the stub: a stub immediate is a
   second, separately encoded statement of the host RVA (`module_base + imm32`).
2. Sixteen rows have no name in the table at all and remain unresolved by this method.

Worked example, Unlock Icons (tag 32, `0x020`): both hooks resolve by symbol and both have a
stub, so the target is confirmed twice — `0x180148B82` → `0x17C500`
(`GameManager::isIconUnlocked`) and `0x180148932` → `0x17C8E0` (`GameManager::isColorUnlocked`).
See [unlock-bypasses.md](unlock-bypasses.md).

## Deliberately not pursued

The import list includes Geode's web utility in full: `WebRequest`, `WebResponse`,
`WebFuture`, `MultipartForm`, `WebRequest::get`, `WebRequest::bodyMultipart`,
`WebResponse::code`, `WebResponse::string`, plus 27 `WS2_32` Winsock imports. Mega Hack
therefore performs outbound HTTP.

This is **recorded as present and left alone.** [AGENTS.md](../AGENTS.md) forbids reversing
licence, DRM or entitlement checks, and a networked code path in a commercial mod is exactly
where those live. The reconstruction does not need it, and no effort goes here until
somebody establishes a legitimate need.

## Conclusion

1. Two-layer host ABI: 1,010 static imports covering cocos2d + Geode + std + Win32 + FMOD,
   and a runtime name-resolution path for everything GD.
2. The runtime path — member-name strings in `.rdata` at `0x18057AEFC` and following, driving
   a repeated registration sequence — is the key to all 160+ features and should be M2's
   first target.
3. Mega Hack links against **Geode's** `cocos2d::CCNode` extensions, not stock cocos2d.
4. Menu UI is `CCLabelBMFont` + `CCMenu`/`CCMenuItem` with cocos2d actions for animation.
5. Geode's `DestructorLock` plus `CCNode` user-object/flag accessors are how Mega Hack
   manages node lifetime and attaches state.
6. Mega Hack performs outbound HTTP via Geode's web utility. Recorded, not investigated.

## PlayLayer method surface (enumerated 2026-10-20)

`_xrefs.tsv` and `_hook_sites.tsv` agree on 26 of the 27 PlayLayer methods Mega Hack
touches; the 27th (`PlayLayer::getCurrentPercent`) is hook-registered but its name
string is not in `_xrefs.tsv` (a Ghidra symbol-coverage gap, not an absence). Counts:

| PlayLayer method | string refs | hook sites |
| ---------------- | ----------: | ---------: |
| `addObject` | 5 | 5 |
| `applyCustomEnterEffect` | 1 | 1 |
| `applyEnterEffect` | 1 | 1 |
| `createCheckpoint` | 1 | 1 |
| `destroyPlayer` | 8 | 8 |
| `getCurrentPercent` | 0 | 1 |
| `init` | 10 | 10 |
| `levelComplete` | 2 | 2 |
| `onQuit` | 1 | 1 |
| `pauseGame` | 2 | 2 |
| `playEndAnimationToPos` | 2 | 2 |
| `playPlatformerEndAnimationToPos` | 2 | 2 |
| `postUpdate` | 11 | 11 |
| `resetLevel` | 25 | 25 |
| `resumeAndRestart` | 1 | 1 |
| `setupHasCompleted` | 5 | 5 |
| `showCompleteEffect` | 1 | 1 |
| `showEndLayer` | 1 | 1 |
| `showNewBest` | 1 | 1 |
| `startGame` | 2 | 2 |
| `storeCheckpoint` | 1 | 1 |
| `toggleProgressbar` | 1 | 1 |
| `updateAttempts` | 1 | 1 |
| `updateProgressbar` | 2 | 2 |
| `updateTestModeLabel` | 1 | 1 |
| `updateTimeWarp` | 1 | 1 |
| `updateVisibility` | 2 | 2 |

Total: 27 methods, 91 string references, 92 hook registrations. Every method that
appears in both indexes has matching counts, which is a cross-check on both.

UNVERIFIED: what each method's callback does. Counts and registration are
evidence-backed; semantics require reading the 92 callbacks, which is M3 work.

## Open questions

- (Resolved 2026-10-20.) `0x18019A0C0` and `0x1800AB850` are the two registration helpers:
  both call `geode::hook::createConvention((TulipConvention)...)` and then
  `FUN_180108E70(resolver, target, callback, method, order, 1)`. The six arguments are
  resolver, target, callback, diagnostic method name, ordering, and a trailing 1. See
  notes/hook-dispatch.md.
- (Resolved 2026-10-20.) The `.data` mangled type names (`GJGameLevel@@...`) are not RTTI
  TypeDescriptors: the 16 bytes preceding `0x1807F7D67` read as ASCII `VPlayLayer@@PEAV`,
  not the two standard `TypeDescriptor` pointers. They are exception-handling type-match
  strings (or compiler-emitted type_info names), not a vtable-backed RTTI object. UNVERIFIED:
  which compiler/exception mechanism consumes them.
- (Resolved 2026-10-20.) The `mov edx, 0x16c8b0` / `0x3a7a80` / `0x3b8eb0` immediates
  are hard-coded host RVAs: they are the values passed to `FUN_18010A370(GetModuleHandleA(NULL)
  base, rva)` and are independently re-read by the lazy thunks at `FUN_1801462C0`,
  `FUN_1801A5190` and `FUN_1801A4300`. They are absolute host-image offsets, not handle
  indices. See notes/hook-dispatch.md.
- (Deferred 2026-10-20.) The 116 `CCNode` and 59 `CCSprite` call inventory is M3 work; it
  is not needed to close M1's host-ABI criterion, which is satisfied by the method-name and
  hook surfaces already enumerated. UNVERIFIED: which of those calls are Mega Hack's own UI
  code versus generic cocos2d usage.
- (Resolved 2026-10-20.) The 26 `PlayLayer::...` method strings that appear in
  `_xrefs.tsv` (91 references) plus the one name-resolved method (`getCurrentPercent`)
  are enumerated in the PlayLayer method surface table above; counts match between
  `_xrefs.tsv` and `_hook_sites.tsv` for every shared method. UNVERIFIED: the semantics of
  each callback, which is M3 work.

## Failure modes recorded

- PowerShell mangles `$` inside MSVC mangled names when passed as command-line arguments to
  `undname.exe`, silently producing a *valid but wrong* demangling. Always pass a file.
- `undname.exe` stdin mode is not supported; it wants arguments or a filename.
- A hand-rolled MSVC mangling parser threw `IndexError` on symbols whose argument list is
  empty. It was replaced with `undname.exe` rather than patched, because a wrong demangling
  that still parses is worse than a crash.

## Resolving the `FF 25` import thunks (2026-10-25)

Most detours reach cocos2d and the CRT through one-instruction thunks near the end of
`.text`:

```
0x180532410  jmp qword ptr [rip + 0x111312]   ; -> IAT slot 0x180643728
```

An address like that says nothing about the behaviour being hooked, and guessing from the
argument shape is exactly how a plausible-but-wrong mechanism gets written down. Every one of
them is resolvable statically, because the IAT slot is named in the import directory.

`scripts/import_names.py` walks `IMAGE_DIRECTORY_ENTRY_IMPORT`, joins each `FirstThunk + n*8`
slot to its DLL and symbol, then joins every `FF 25 <disp32>` site in `.text` to the slot it
jumps through. Self-check on the baseline artifact:

```
imports: 1010 across 30 DLLs; FF 25 stubs found: 894
```

1,010 imports over 30 DLLs is the same figure this note recorded from `pefile` on 2026-10-20,
so the parser agrees with the earlier tool on the total before being used for anything.
701 of the 894 `FF 25` hits land on an import slot; the remaining 193 are byte pairs inside
other instructions, which is why the tool reports a stub only when its slot resolves.

Four examples, all from detours in this repository:

| Thunk | Resolves to |
| --- | --- |
| `0x180532410` | `libcocos2d.dll!?count@CCArray@cocos2d@@QEBAIXZ` |
| `0x1805330B0` | `libcocos2d.dll!?clear@CCDrawNode@cocos2d@@QEAAXXZ` |
| `0x1805331B0` | `libcocos2d.dll!?drawPolygon@CCDrawNode@cocos2d@@QEAA_NPEAVCCPoint@2@IAEBU_ccColor4F@2@M1W4BorderAlignment@2@@Z` |
| `0x1805323C0` | `libcocos2d.dll!?createWithVariableList@CCSequence@cocos2d@@SAPEAV12@PEAVCCFiniteTimeAction@2@PEAD@Z` |

`scripts/test_import_names.py` pins the totals and those four resolutions, and skips itself
when the sample set is absent.

### Failure modes recorded

- Reading the displacement as an absolute address rather than RIP-relative is wrong by the
  instruction length; the stub above lands on `0x180643728`, not `0x180532410 + 0x111312`.
- Treating every `FF 25` byte pair in `.text` as a thunk over-reports: 894 hits, 701 real
  slots. A resolution failure is data, not an error to paper over.
