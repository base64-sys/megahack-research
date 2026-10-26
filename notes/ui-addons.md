# UI add-on hooks cluster

- **Question** — which UI and menu paths does Mega Hack hook, and what do they
  share?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Filter `recovered/functions/_hook_sites.tsv` for GD methods whose names contain
   `Layer`, `Menu`, `Pause`, `Options`, `EndLevel`, `RewardUnlock`, `Creator`,
   `LevelInfo`, `LevelSelect`, `UILayer`, `VideoOptions`, or `EditorPause`.
2. Cross-check registrar and callback VAs against `notes/hook-dispatch.md`.

## Evidence — UI hook sites

| Registrar VA | GD method | Target | Callback VA | Callback kind | Helper |
| --- | --- | --- | --- | --- | --- |
| `0x1800A8020` | `MenuLayer::init` | RVA `0x333A90` | `0x18010B020` | function | `0x1800AA290` |
| `0x1800A8020` | `GJGarageLayer::init` | RVA `0x273050` | `0x18010B060` | function | `0x1800AA430` |
| `0x1800A8020` | `PauseLayer::customSetup` | RVA `0x37C870` | `0x18010B400` | function | `0x1800AA5D0` |
| `0x1800A8020` | `PauseLayer::onResume` | RVA `0x37E380` | `0x18010B4A0` | function | `0x1800AA770` |
| `0x1800BA9D0` | `MenuLayer::init` | RVA `0x333A90` | `0x1800BAA70` | function | `0x1800AA290` |
| `0x180151280` | `LevelEditorLayer::init` | RVA `0x2D1DD0` | `0x180151420` | function | `0x180151640` |
| `0x180151280` | `EditorPauseLayer::onSaveAndPlay` | RVA `0xDDEE0` | `0x180151440` | function | `0x180151890` |
| `0x180151280` | `EditorPauseLayer::onSaveAndExit` | RVA `0xDE260` | `0x180151470` | function | `0x180151890` |
| `0x180151280` | `EditorPauseLayer::onExitNoSave` | RVA `0xDE370` | `0x1801514A0` | function | `0x180151890` |
| `0x1801534F0` | `LevelSelectLayer::init` | RVA `0x3183F0` | `0x1801535D0` | function | `0x180154250` |
| `0x1801534F0` | `LevelAreaInnerLayer::init` | RVA `0x2C64E0` | `0x180153830` | function | `0x180154430` |
| `0x180154AD0` | `MenuLayer::init` | RVA `0x333A90` | `0x180154C10` | function | `0x1800AA290` |
| `0x18015E220` | `CreatorLayer::onSecretVault` | RVA `0x9E590` | `0x18015E2F0` | function | `0x18015DA10` |
| `0x18015E220` | `OptionsLayer::onSecretVault` | RVA `0x372CD0` | `0x18015E330` | function | `0x18015E370` |
| `0x18015E920` | `LevelInfoLayer::init` | RVA `0x2F7340` | `0x18015E9F0` | function | `0x18015EAB0` |
| `0x18015FD40` | `PauseLayer::customSetup` | RVA `0x37C870` | `0x18015FE70` | function | `0x1800AA5D0` |
| `0x18015FD40` | `PauseLayer::onTryEdit` | RVA `0x37E550` | `0x18015FEC0` | function | `0x1800AA770` |
| `0x18015FD40` | `EditorPauseLayer::init` | RVA `0xD9EC0` | `0x18015FF20` | function | `0x18015FF70` |
| `0x1801601F0` | `LevelEditorLayer::init` | RVA `0x2D1DD0` | `0x180160440` | function | `0x180151640` |
| `0x180167D10` | `UILayer::init` | RVA `0x4CCB00` | `0x180167D80` | function | `0x180167DB0` |
| `0x1801696E0` | `EndLevelLayer::customSetup` | RVA `0x132E50` | `0x1801697B0` | function | `0x180169890` |
| `0x180175640` | `LevelInfoLayer::init` | RVA `0x2F7340` | `0x1801756B0` | function | `0x18015EAB0` |
| `0x180176FF0` | `PauseLayer::onNormalMode` | RVA `0x37E2A0` | `0x180177060` | function | `0x1800AA770` |
| `0x1801772E0` | `PauseLayer::onPracticeMode` | RVA `0x37E1C0` | `0x180177350` | function | `0x1800AA770` |
| `0x1801774B0` | `PauseLayer::onRestart` | RVA `0x37E410` | `0x180177520` | function | `0x1800AA770` |
| `0x180177680` | `PauseLayer::onRestartFull` | RVA `0x37E4B0` | `0x1801776F0` | function | `0x1800AA770` |
| `0x18017ECB0` | `EndLevelLayer::onReplay` | RVA `0x134E80` | `0x18017ED20` | function | `0x18017ED60` |
| `0x180184490` | `UILayer::init` | RVA `0x4CCB00` | `0x180184710` | function | `0x180167DB0` |
| `0x180186100` | `PauseLayer::musicSliderChanged` | RVA `0x37E130` | `0x180186290` | function | `0x1800AA770` |
| `0x180186100` | `PauseLayer::sfxSliderChanged` | RVA `0x372A90` | `0x1801862C0` | function | `0x1800AA770` |
| `0x180186100` | `OptionsLayer::musicSliderChanged` | RVA `0x372990` | `0x1801862F0` | function | `0x18015E370` |
| `0x180186100` | `OptionsLayer::sfxSliderChanged` | RVA `0x372A90` | `0x180186370` | function | `0x18015E370` |
| `0x180187820` | `RewardUnlockLayer::init` | RVA `0x3CDDF0` | `0x180187CE0` | function | `0x180187D80` |
| `0x180187820` | `RewardUnlockLayer::playRewardEffect` | RVA `0x3CEDE0` | `0x180187D00` | function | `0x180187FD0` |
| `0x180187820` | `RewardUnlockLayer::showCloseButton` | RVA `0x3D0480` | `0x180187D20` | function | `0x180187FD0` |
| `0x180189A70` | `LevelAreaInnerLayer::init` | RVA `0x2C64E0` | `0x180189FA0` | label | `0x180154430` |
| `0x18018C040` | `GJListLayer::init` | RVA `0x27F8D0` | `0x18018C110` | function | `0x18018C1A0` |
| `0x18018E660` | `PauseLayer::onResume` | RVA `0x37E380` | `0x18018E830` | function | `0x1800AA770` |
| `0x1801985A0` | `EndLevelLayer::customSetup` | RVA `0x132E50` | `0x1801985F0` | function | `0x180169890` |
| `0x18014ED70` | `VideoOptionsLayer::onApply` | RVA `0x378490` | `0x18014FA80` | function | `0x180150370` |

## Pattern

- `MenuLayer::init` is registered three times (`0x1800A8020`, `0x1800BA9D0`,
  `0x180154AD0`) and all converge on helper `FUN_1800AA290`.
- `PlayLayer::init` is registered at least seven times (`0x1800A83B0`,
  `0x1801551F0`, `0x1801582C0`, `0x1801718C0`, `0x180175730`,
  `0x180177960`, `0x1801787C0`, `0x180179A10`, `0x18017A180`,
  `0x1801812F0`, `0x180184490`) and all converge on helper `FUN_1800AB5B0`.
- `PauseLayer::customSetup` is registered twice (`0x1800A8020`,
  `0x18015FD40`) and both converge on helper `FUN_1800AA5D0`.
- `PauseLayer` mode and slider changes are registered repeatedly
  (`onNormalMode`, `onPracticeMode`, `onRestart`, `onRestartFull`,
  `musicSliderChanged`, `sfxSliderChanged`) and converge on helper
  `FUN_1800AA770`.
- `LevelEditorLayer::init` is registered three times (`0x180151280`,
  `0x1801601F0`, `0x1801812F0`) and all converge on helper
  `FUN_180151640`.
- `UILayer::init` is registered twice (`0x180167D10`, `0x180184490`) and both
  converge on helper `FUN_180167DB0`.
- `EndLevelLayer::customSetup` and `onReplay` share helper
  `FUN_180169890` / `FUN_18017ED60`.

The recurring shape is the same as the input and gameplay clusters: Mega Hack
observes the same UI event from several ownership contexts and funnels each
family through one helper.

## Open questions

- UNVERIFIED: which settings keys gate each UI add-on. The callback bodies do not
  expose their tags in the recovered snippets used here.
- UNVERIFIED: the behaviour of shared helpers `FUN_1800AA290`,
  `FUN_1800AA5D0`, `FUN_1800AA770`, `FUN_1800AB5B0`, `FUN_180151640`,
  `FUN_18015E370`, `FUN_180167DB0` and `FUN_180169890`.
- UNVERIFIED: whether `RewardUnlockLayer` and `CreatorLayer` hooks are feature
  hooks or incidental registrations.
- UNVERIFIED: whether any of these hooks can veto the original call, or only
  observe it.

## Evidence — Transition Customiser (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18018B7F0` (`0x18018B7F0`) is hooked on `cocos2d::CCTransitionFade::create`
(registered by `FUN_18018AEB0` at `0x18018AEB0` via `FUN_180189FB0` with enum arg
`0xFFFFF428`):

- If `isHackEnabled(HACK_NO_TRANSITION = 449, 0x1C1)` is true: calls
  `cocos2d::CCTransitionFade::create(param_1, param_2)` directly (no custom
  opacity/duration).
- If false: reads opacity from tag 0x1D8 (472) via `FUN_180003D80` (double
  accessor) and duration from tag 0x1D6 (470) via `FUN_18002E4C0` (uint/int
  accessor), then calls `FUN_18018B930(duration, opacity, scene)`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1C1 | 449 | `isHackEnabled` (Map B) | `HACK_NO_TRANSITION` | Master toggle (disables custom transitions) |
| 0x1D8 | 472 | `FUN_180003D80` (double accessor) | (not in static list) | Transition opacity (0–100) |
| 0x1D6 | 470 | `FUN_18002E4C0` (uint/int accessor) | (not in static list) | Transition duration

UNVERIFIED: which JSON keys bind to tags 0x1D6 and 0x1D8; the behaviour of
`FUN_18018B930`; whether the toggle is "disable all transitions" or "use
default transition".

## Evidence — Interface tab / hide UI changes (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_1801985A0` (`0x1801985A0`) registers `FUN_1801985F0` (`0x1801985F0`)
as a hook on `EndLevelLayer::customSetup` (RVA `0x132E50`).

`FUN_1801985F0` modifies the end-level screen:
- If `isHackEnabled(tag 0x1F0 = 496)` is true: returns immediately (no UI changes).
- If false: finds "main-layer" and "background" nodes, adds a "v9.1.3" label
  (`CCLabelBMFont::create("v9.1.3", "chatFont.fnt")`) and a "." label, modifying
  the end-level screen appearance.

So tag 0x1F0 (496) is the "disable Mega Hack UI changes" toggle: when enabled,
the version branding label is not added.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1F0 | 496 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | "Disable Mega Hack UI changes" toggle

UNVERIFIED: which JSON key binds to tag 0x1F0; whether this is the "Interface tab"
feature or a different UI-change toggle; the full set of UI modifications gated
by this tag (only the EndLevelLayer one was observed).

## Evidence — Language-key-driven toggle (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180197DA0` (`0x180197DA0`) registers two hooks:

1. `FUN_180197EA0` on `GameManager::setGameVariable` (RVA `0x182EB0`): when
   the language dictionary is loaded and key "0025" is found, toggles hack tag
   0x1ED (493) (XOR with 1). When key "0170" is found, sets config tag
   0x1EE (494) if it differs.
2. `FUN_180197F40` as a change listener for tag 0x1EE (494): when the value
   changes, if `FUN_1800BB170()` is true and the language key "0025" state
   differs, sets the language key and calls `FUN_18014E6A0` (UI update).

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1ED | 493 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Toggled by language key "0025" |
| 0x1EE | 494 | `FUN_180027ED0` (config write) / listener | (not in static list) | Set by language key "0170"; drives UI update

UNVERIFIED: which feature this corresponds to (candidate: multi-lingual support);
the semantics of `FUN_1800BB170` and `FUN_18014E6A0`; the language keys
"0025" and "0170" and what they represent.

## Evidence — confirmation dialogs and Place Over (2026-10-25)

Four `HACK_CONFIRM_*` tags and `HACK_PLACE_OVER` are the first toggles whose behaviour is
settled by the detour body rather than by the target name alone.

| Tag | Static label | Registration | Callback | Target | Detour |
| ---: | --- | --- | --- | --- | --- |
| `0x033` | `HACK_PLACE_OVER` | `0x1800B0511` | `FUN_180163000` | `LevelEditorLayer::typeExistsAtPosition` RVA 0x2D4A50 | `0x180163070` |
| `0x137` | `HACK_CONFIRM_PRACTICE` | `0x1800B2273` | `FUN_1801772E0` | `PauseLayer::onPracticeMode` RVA 0x37E1C0 | `0x180177350` |
| `0x138` | `HACK_CONFIRM_NORMAL` | `0x1800B21FC` | `FUN_180176FF0` | `PauseLayer::onNormalMode` RVA 0x37E2A0 | `0x180177060` |
| `0x139` | `HACK_CONFIRM_RESET` | `0x1800B22EA` | `FUN_1801774B0` | `PauseLayer::onRestart` RVA 0x37E410 | `0x180177520` |
| `0x13A` | `HACK_CONFIRM_FULL_RESET` | `0x1800B2361` | `FUN_180177680` | `PauseLayer::onRestartFull` RVA 0x37E4B0 | `0x1801776F0` |

**Correction (2026-10-25):** the registration and callback addresses in this table were
first written from recall and four of them were wrong — `0x1800B028E`/`FUN_180162B60` for
Place Over, and the `0x1800B1A**` range for the confirm tags. They are now taken from
`scripts/tag_callback_map.py` and re-checked against the instruction bytes, which is the
only acceptable source: `0x1800B0511` begins `ba 33 00 00 00` (the tag) and `0x1800B2273`
begins `ba 37 01 00 00`. A registration VA in a note is a measurement, not something to
type from memory.

### Place Over returns "not occupied"

`LevelEditorLayer::typeExistsAtPosition` is replaced wholesale:

```
0x180163070  xor  eax, eax
0x180163072  ret
```

The editor calls this predicate before allowing a placement, so forcing it to return `false`
means "nothing of this type occupies the position" — the direct mechanism for placing an
object over an existing one. This is decisive, not inferred: no argument is read.

### The confirmation wrapper

The four `HACK_CONFIRM_*` detours are the same shape, differing only in the tail-call
target. Practice mode, in full:

```
0x180177364  cmp  byte [rip+0x6952CD], 1     ; guard -> 0x18080C638
0x18017736B  jne  0x180177379
0x180177374  jmp  0x1801A3080                ; guard set: original handler
0x180177379  ...
0x180177381  call 0x1803A50B4                ; allocate 0x240 bytes
0x18017738D  call 0x1801E7F50                ; construct dialog
0x180177392  lea  rax,[rip+0x3FEFEF]         ; vtable writes at +0x00, +0x140,
...                                          ; +0x148 ... +0x230
0x180177410  mov  [rdi+0x228], rsi           ; dialog keeps the layer
```

| Detour | Guard | Handler on the guarded path |
| --- | --- | --- |
| `0x180177350` practice | `0x18080C638` | `0x1801A3080` |
| `0x180177060` normal | `0x18080C638` | `0x1801A2FB0` |
| `0x180177520` restart | `0x18080C638` | `0x1801A3150` |
| `0x1801776F0` full reset | `0x18080C638` | `0x1801A3220` |

All four read the **same** guard byte, which is the reusable "dialog already answered"
flag: when it is set the detour tail-jumps straight to the original action, and otherwise it
constructs a 0x240-byte dialog with the action's own vtable and callbacks, keeping the layer
at `+0x228`. The per-feature gating is done by the callback, which installs the detour only
on the `*param_2 == 1` path — so there is one wrapper implementation and four settings.

`HACK_CONFIRM_NORMAL` has no changelog row of its own, and `settings-contents.md` only
records `LEVEL/HACK_CONFIRM_FULL_RESET` and `LEVEL/HACK_CONFIRM_NORMAL` among the user's
202 keys, so `HACK_CONFIRM_PRACTICE`/`_RESET` keys are **UNVERIFIED**.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x033 | 51 | `isHackEnabled` (Map B) | `HACK_PLACE_OVER` | Place objects over existing ones |
| 0x137 | 311 | `isHackEnabled` (Map B) | `HACK_CONFIRM_PRACTICE` | Confirm before Practice Mode |
| 0x138 | 312 | `isHackEnabled` (Map B) | `HACK_CONFIRM_NORMAL` | Confirm before Normal Mode |
| 0x139 | 313 | `isHackEnabled` (Map B) | `HACK_CONFIRM_RESET` | Confirm before restart |
| 0x13A | 314 | `isHackEnabled` (Map B) | `HACK_CONFIRM_FULL_RESET` | Confirm before full reset |

### Who sets the guard

A `.text` scan for rip-relative references to `0x18080C638` finds exactly **six**: the four
`cmp` instructions above, plus one writer pair at `0x1801778C9` (set to 1) and `0x1801778E4`
(clear to 0) inside `FUN_1801778C0`:

```
0x1801778C0  sub  rsp, 0x28
0x1801778C4  test r8b, r8b
0x1801778C7  je   0x1801778EB
0x1801778C9  mov  byte [0x18080C638], 1   ; answering the dialog
0x1801778D0  movsxd rax, [rcx+0x18]
0x1801778D4  add  rax, [rcx+8]
0x1801778DE  xor  edx, edx
0x1801778E0  call qword [r8+0x10]         ; the deferred action
0x1801778E4  mov  byte [0x18080C638], 0
```

So the guard is a **scoped re-entrancy flag around the confirmed action**, not a per-setting
switch: the wrapper sets it, invokes the stored delegate, and clears it. The four detours
therefore take the direct path only while that delegate is running — which is precisely the
"the user already confirmed" case. `FUN_1801778C0` has no direct `call` site; its only
reference is an absolute pointer stored at `0x180576910` (a static descriptor table, not a
vtable slot: the qword before it is `0x180576920`, not a COL).

UNVERIFIED: the dialog class behind `0x1801E7F50` and the descriptor at `0x180576388`;
which JSON keys bind to three of the four confirm tags.

## Evidence — Text Length Bypass (2026-10-25)

`HACK_TEXT_LENGTH` (tag 37, `0x25`, static-map entry #72, store VA `0x180064420`) is
registered at `0x1800AFE13`; callback `FUN_18015D6D0` hooks
`CCTextInputNode::onTextFieldAttachWithIME` (RVA `0x508F0`) with detour `0x18015D740`,
installer `0x18015D770`.

The detour raises the input's length cap to `INT_MAX` before the original runs:

```
0x18015D74C  add   rcx, -0x1A0              ; the owning node, not the input
0x18015D753  mov   edx, 0x7FFFFFFF
0x18015D758  call  0x1801E6D20              ; set the cap
0x18015D769  jmp   0x1801EB310              ; original onTextFieldAttachWithIME
```

The choke point is chosen well: the limit is re-applied every time the text field gains IME
focus, so it also covers fields created after the toggle was enabled. `INT_MAX` rather than a
large finite value means the cap is effectively removed.

UNVERIFIED: which field `0x1801E6D20` writes (the `rcx-0x1A0` back-reference is inferred from
this one detour); the JSON key for tag 0x25.

## Evidence — No Transition (2026-10-25)

`HACK_NO_TRANSITION` (tag 449, `0x1C1`; entry #47 in the static label map, store VA
`0x180063A90`) registers at `0x1800B317B` (`ba c1 01 00 00 48 89 c1 49 89 f0 e8 …`); the
callback is `FUN_180189A70`. It installs five hooks — two resolved by mangled symbol, three by
RVA:

| Target | Resolution key | Detour | Installer |
| --- | --- | --- | --- |
| `cocos2d::CCTransitionFade::create` | `?create@CCTransitionFade@cocos2d@@SAPEAV12@MPEAVCCScene@Z` (string `0x180579934`) | `0x180189F50` | `0x180189FB0` |
| `cocos2d::CCTransitionMoveInT::create` | `?create@CCTransitionMoveInT@cocos2d@@SAPEAV12@MPEAVCCScene@Z` (`0x180579996`) | `0x180189F60` | `0x18018A190` |
| `GJShopLayer::onBack` | RVA `0x2ACD70` | `0x180189F70` | `0x18018A370` |
| `LevelAreaLayer::onClickDoor` | RVA `0x2C55C0` | `0x180189F90` | `0x18018A550` |
| `LevelAreaInnerLayer::init` | RVA `0x2C64E0` | `0x180189FA0` | `0x180154430` |

The two transition factories are the decisive evidence, and they are the same trick twice:

```
0x180189F50  xorps  xmm0, xmm0        ; duration = 0.0f
0x180189F53  jmp    0x180532EA0       ; CCTransitionFade::create body

0x180189F60  xorps  xmm0, xmm0        ; duration = 0.0f
0x180189F63  jmp    0x1805332C0       ; CCTransitionMoveInT::create body
```

Both factories take `(float duration, CCScene*)`, so zeroing `xmm0` keeps the fade/move-in
wrapper but removes the animation. The three menu hooks cover the remaining transition entry
points, which is exactly what the changelog fix bullet describes — "No Transition now works on
shop, tower & garage layers":

- `LevelAreaInnerLayer::init` detour is `mov dl,1; jmp 0x18014CF80`, i.e. it calls the original
  init with second argument `1`.
- `LevelAreaLayer::onClickDoor` detour is a bare forward: `jmp 0x18014D3E0`.
- `GJShopLayer::onBack` detour calls `0x180146F20`, then tail-jumps `vtable+0x200`.

Those three forward into `0x18014CF80` / `0x18014D3E0` / `0x180146F20`. The earlier
"MSVC incremental-link thunk" description is **withdrawn**: reading the bodies shows a
lazy-binding stub, not an ILT shim. In full, `0x18014CF80`:

```
0x18014CF94  mov  eax, [rip+...]          ; cached generation counter
0x18014CF9A  mov  r8d, [rip+...]          ; per-thread slot index
0x18014CFA1  mov  r9, qword gs:[0x58]     ; thread-local block
0x18014CFAA  mov  r8, [r9+r8*8]           ; per-thread table
0x18014CFAE  cmp  eax, [r8+0x10]          ; already resolved?
0x18014CFC5  jmp  rax                     ; cached target, fast path
0x18014CFD7  call 0x1803A5140             ; slow path: resolver
0x18014CFE1  cmp  dword [rip+...], -1     ; ...
```

The slow path obtains a module base from `0x18010DDC0`, adds a fixed offset
(`lea rcx,[rax+0x2C57E0]` at `0x18014D444`), calls `0x18014D230`, and caches the result.
That is **MH's own lazy host-symbol binder**, not a PE delay-load table: the image's
delay-import data directory (index 13) is **empty** (`rva=0 size=0`), while the IAT
directory (index 12) is `0x642CB8`/`0x2080`. The stub family is large — `mov r9, gs:[0x58]`
appears **81** times and the sibling `mov r8, gs:[0x58]` **110** times in the image — so MH
reaches host functions through a per-thread cache that is populated on first call.

The consequence for feature mapping is concrete: the detour calls *MH's own* entry point for
the original function, so the detour body tells us the argument it passes. The host address is
recoverable statically — the binder's base is `GetModuleHandleA(NULL)` and the stub adds the
host RVA — so `LevelAreaInnerLayer::init` reaches host RVA `0x2C64E0` and `GJShopLayer::onBack`
`0x2ACD70`, both matching the names the hooks were registered with. `onClickDoor`'s stub adds
`0x2C57E0`, while the callback names `0x2C55C0`; that `0x220` disagreement is recorded as
**UNVERIFIED** in [host-abi.md](host-abi.md#one-disagreement-left-open) rather than smoothed
over. Whether `dl=1` suppresses the transition remains **UNVERIFIED**.

### Scope note

No Transition has **no `Added …` row**, so it is not in the 389-feature universe. It appears in
the changelog only as a fix bullet, which widens the M3 scope gap recorded in
[feature-inventory.md](feature-inventory.md#toggles-outside-the-389) beyond `New For 2.2 Mods`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1C1 | 449 | `isHackEnabled` (Map B) | `HACK_NO_TRANSITION` | Master toggle (No Transition) |

UNVERIFIED: the JSON key for tag 449; whether the `vtable+0x200` tail call in `onBack`
reaches the original handler; whether `dl=1` skips the transition. The binder and the host
RVAs are described in
[host-abi.md](host-abi.md#how-mega-hack-calls-host-functions-it-does-not-import).

## Evidence — Auto Song Downloader (2026-10-25)

Tag 323 (`0x143`, `HACK_AUTO_SONG_DOWNLOAD`, static-map entry 7, store VA `0x180062C24`)
is registered at `0x1800B1FA4`. Callback `FUN_180175640` installs one hook, and both the
callback and the detour live in code caves — `scripts/func_bounds.py` returns no `.pdata`
record for either address:

| Target | RVA | Detour | Installer | Order |
| --- | ---: | --- | --- | --- |
| `LevelInfoLayer::init` | `0x2F7340` | `0x1801756B0` | `0x18015EAB0` | `0xFFFFFFE0` |

The detour is a **post-hook**: it runs the original first and only then does its own work,
returning the original's bool unchanged.

```
0x1801756B6  mov  rsi, rcx
0x1801756B9  call 0x1801A0E60          ; gateway -> host RVA 0x2F7340, the original init
0x1801756BE  test al, al
0x1801756C0  je   0x180175712          ; init failed -> return its false
0x1801756C2  mov  rcx, [rsi+0x2D0]
0x1801756C9  test rcx, rcx
0x1801756CC  je   0x180175712          ; member not set -> nothing to do
0x1801756CE  mov  rcx, [rcx+0x180]
0x1801756D5  mov  rdx, [rcx]           ; vtable of that sub-object
0x1801756D8  mov  ebx, eax             ; remember init's result
0x1801756DA  call [rdx+0x148]          ; virtual predicate, result in cl
0x1801756E4  test cl, cl
0x1801756E6  je   0x180175712          ; predicate false -> stop
0x1801756E8  mov  rcx, [rsi+0x2D0]
0x1801756EF  mov  rsi, [rcx+0x180]     ; the same sub-object
0x1801756F6  lea  rdx, [0x18010E630]   ; gateway -> host RVA 0xCABC0
0x1801756FD  mov  r8, rsi              ; target and argument are the same object
0x180175700  call 0x180533230          ; cocos2d::CCCallFuncO::create
0x180175705  mov  rcx, rsi
0x180175708  mov  rdx, rax
0x18017570B  call 0x180532CE0          ; cocos2d::CCNode::runAction
0x180175710  mov  eax, ebx
0x180175712  add rsp, 0x28 ; pop rsi ; pop rbx ; ret
```

Read plainly: once the level-info layer has initialised, if `[layer+0x2D0]` is set and the
sub-object at `+0x180` answers the virtual predicate at vtable slot `+0x148` with true, the
mod queues `CCCallFuncO::create(subObject, <host RVA 0xCABC0>, subObject)` on that same
sub-object and lets cocos2d run it on a later frame. The gateway's own initialiser names the
callee it wraps: `0x18010E6A1 lea rcx, [rax+0xcabc0]`, followed by `call 0x18010E450` to
build the gateway once and cache it at `0x180802EF0`.

The callback's enable branch is the usual record layout: `mov edx, 0x2F7340`,
`call 0x18010A370`, `lea r8` → detour, `lea r9` → `0x1805752BC` (the string
`LevelInfoLayer::init` in `_host_methods.tsv`), `mov dword [rsp+0x20], 0xFFFFFFE0`, then
`call 0x18015EAB0`. That installer is shared with Copy Hack (tag 43, callback
`FUN_18015E920`, detours `0x18015E9F0` and `0x18015EA50`), which hooks the same RVA — it is a
generic order-taking installer, not part of this feature.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x143 | 323 | `isHackEnabled` (Map B) | `HACK_AUTO_SONG_DOWNLOAD` | Deferred call into host RVA `0xCABC0` after `LevelInfoLayer::init` |

UNVERIFIED: what `LevelInfoLayer+0x2D0` holds and what its `+0x180` sub-object is (the
offsets are measured, but no host symbol names either field); what vtable slot `+0x148`
returns for that class; and what host RVA `0xCABC0` does — no name string in the image refers
to it, it is absent from `_host_methods.tsv`, and the only occurrence of the immediate is the
gateway initialiser above. The "auto download" reading therefore rests on the changelog
wording plus the deferred-call shape, not on an identified download routine.

## Evidence — Toolbox Button Bypass (2026-10-25)

One changelog row, one tag, two hooks: both stepper handlers of the editor options layer are
replaced outright, and neither detour calls the function it replaces. That is the whole
mechanism, so the row is `mapped`; the vanilla ceilings themselves are host code and stay
`UNVERIFIED`.

| Field | Value |
| --- | --- |
| Tag | 47 (`0x02F`, `HACK_TOOLBOX_BUTTON_BYPASS`) |
| Static label | map entry 74, store `0x1800644EA` (`notes/static-hack-label-map.md`) |
| Registration | `0x1800B0604` (`notes/hook-registrations.md`) |
| Callback | `FUN_180163F50` |
| Installer | `0x180164340` (both hooks, two calls at `0x180163FA0` and `0x180163FFC`) |
| Hook 1 | `EditorOptionsLayer::onButtonRows`, RVA `0x291A00` -> detour `0x180164020` |
| Hook 2 | `EditorOptionsLayer::onButtonsPerRow`, RVA `0x291970` -> detour `0x1801641B0` |

### Detour 1 — `onButtonRows`, `0x180164020`

Code cave (no `.pdata` record). Body `0x180164020..0x180164141`, `ret` at `0x180164141`,
0x122 bytes, then `ud2` at `0x180164142`; the SEH funclet at `0x180164150` is not part of the
flow.

```
0x180164020  push rbp; push rsi; push rdi; sub rsp,0x90; lea rbp,[rsp+0x80]
0x180164032  mov qword [rbp+8], 0xFFFFFFFFFFFFFFFE   ; SEH frame marker
0x18016403A  mov rsi, rcx                            ; this = EditorOptionsLayer
0x18016403D  mov rax, [rdx]                          ; stepper argument object
0x180164040  mov rcx, rdx
0x180164043  call [rax+0x38]                         ; direction -> eax
0x180164046  mov ecx, [rsi+0x3F4]                    ; current rows value
0x18016404C  cmp eax, 1
0x18016404F  jne 0x180164055
0x180164051  inc ecx                                ; direction 1: increment, no ceiling
0x180164053  jmp 0x180164064
0x180164055  cmp ecx, 3
0x180164058  mov eax, 2
0x18016405D  cmovge eax, ecx
0x180164060  dec eax                                ; floor: max(ecx,3)-1
0x180164062  mov ecx, eax
0x180164064  mov [rsi+0x3F4], ecx                    ; store new value
0x18016406A  mov rdi, [rsi+0x400]                    ; label owner
0x180164071  mov [rbp-0x28], ecx                     ; format argument payload
0x180164074  movabs rax, 0x1000000000000000
0x18016407E  mov [rbp-0x30], rax                     ; argument tag word
...
0x180164099  mov qword [rbp-0x50], 1                 ; one format argument
0x1801640A9  lea rax, [0x180542A08]                  ; "{}"
0x1801640B0  mov [rbp-0x40], rax
0x1801640B4  mov qword [rbp-0x38], 2                 ; length 2
0x1801640C5  lea rcx, [rbp-0x58]                     ; out string
0x1801640D8  call 0x18000AE30                        ; format "{}" with the count
0x1801640DD  lea rcx, [rdi+0x168]                    ; label sub-object
0x1801640EF  mov rax, [rdi+0x168]
0x1801640F6  mov rdx, rsi
0x1801640F9  call qword [rax]                        ; vtable slot 0 <- formatted string
0x180164136  add rsp,0x90; pop rdi; pop rsi; pop rbp; ret
```

The unchanged part of the body (`0x1801640DD..0x180164141`) allocates the `std::string`
temporaries, passes the formatted text down and unwinds them; no branch returns to a vanilla
`onButtonRows`, and no gateway initialiser for RVA `0x291A00` appears anywhere in the cave.

### Detour 2 — `onButtonsPerRow`, `0x1801641B0`

Same shape, body `0x1801641B0..0x1801642D1`, `ret` at `0x1801642D1`, 0x121 bytes, `ud2` at
`0x1801642D2`. The SEH funclet at `0x1801642E0` ends at `0x180164304` and must not be quoted
as the body end. Only the member offsets differ:

| | `onButtonRows` | `onButtonsPerRow` |
| --- | --- | --- |
| stored count | `[this+0x3F4]` | `[this+0x3F0]` |
| label owner | `[this+0x400]` | `[this+0x3F8]` |
| label sub-object | `+0x168` | `+0x168` |

Both detours share the direction logic exactly (`cmp eax,1` -> plain `inc`; anything else ->
`max(count,3)-1`), the same `"{}"` format call through `0x18000AE30` with a single argument,
and the same `call [label+0x168]` vtable slot 0. So the toggle raises the maximum of both
editor steppers and never lets either drop below 1.

Supporting reads: `0x180542A08` holds `7b 7d 00` (`"{}"`) followed by `7b 0a 00`, `22 3a 20 00`,
`2c 0a 00`, `22 3a 00` — a table of format fragments, the detours using the first; and
`0x18000AE30` is a large formatting helper (`push rbp/r15/r14/r13/r12/rsi/rdi/rbx`,
`sub rsp,0x1B8`) with no name string referring to it.

UNVERIFIED: the vanilla ceilings and where the host enforces them — the detours replace the
functions instead of post-processing their result, so the original bound is not readable from
this image and the "bypass" wording rests on the changelog plus the unbounded `inc`. Also open:
the identity of the virtual at `[[arg2]+0x38]` (a direction/step selector is inferred from the
`cmp eax,1` branch, not proven); the names of `EditorOptionsLayer+0x3F0`/`+0x3F4` and whether
host code reads them too; and what the objects at `+0x3F8`/`+0x400` are, whose `+0x168`
sub-object's slot 0 is only shape-compatible with a label `setString`.

## Evidence — Slider Bypass (2026-10-26)

Tag 38 (`0x026`, `HACK_SLIDER_LIMIT`, static map entry 65, store `0x180064155`) has one
hook, and its detour is a replacement rather than a wrapper: there is no reference to a
cached original anywhere in the cave.

| Field | Value |
| --- | --- |
| Registration | `0x1800AFD9C` |
| Callback | `FUN_18015D320` (body `0x18015D320..0x18015D378`) |
| Disable branch | `0x18015D379` tail-jumps `0x180108A80` with the same detour |
| Target | `SliderTouchLogic::ccTouchMoved`, RVA `0x715D0` |
| Detour | `0x18015D390`, `.pdata` extent `0x18015D390..0x18015D46C` |
| Installer | `0x18015D470`, order `0xBD8`, sixth argument 1 |
| Name string | `0x1805751AE` "SliderTouchLogic::ccTouchMoved" |

Callback shape is the usual one: `movzx ebx,[rdx]` reads the enable byte, `0x180108960`
opens the settings context, `cmp bl,1` selects install, `0x18010A250` returns the module
base and `0x18010A370` resolves RVA `0x715D0`; `[rsp+0x28]=1` and `[rsp+0x20]=0xBD8` are
the last two installer arguments.

### The detour body

`/GS` cookie from `0x1807F7040`; `rsi = this` then `add rsi,-0x140`, so every later
`[rsi+X]` is `[this+X-0x140]` and the meaningful offsets are `+0x88` and `+0x90`.

```
0x18015d397  mov rsi,rcx
0x18015d3a9  mov rdi,[rcx+0x88]                 ; slider object
0x18015d3b7  cmp byte [rdi+0x194],1
0x18015d3be  jne 0x18015d3f4
0x18015d3c0  lea rbx,[rsp+0x28]; mov rcx,rdx; mov rdx,rbx
0x18015d3cb  call 0x180532B60                   ; CCTouch::getLocation
0x18015d3d5  mov rcx,rsi; lea rdx,[rsp+0x30]; mov r8,rbx
0x18015d3db  call 0x180532B70                   ; CCNode::convertToNodeSpace
0x18015d3e0  movss xmm1,[rsp+0x34]              ; node-space y
0x18015d3e6  mov rax,[rdi]; mov rcx,rdi
0x18015d3ec  call [rax+0xE0]                    ; vertical path
0x18015d3f2  jmp 0x18015d426
0x18015d3f4  ... same two calls ...
0x18015d414  movss xmm1,[rsp+0x30]              ; node-space x
0x18015d41a  mov rax,[rdi]; mov rcx,rdi
0x18015d420  call [rax+0xD0]                    ; horizontal path
0x18015d426  mov rcx,[rsi+0x1D0]                ; = [this+0x90]
0x18015d42d  call 0x1801FF740
0x18015d432  mov rcx,[rsi+0x1C8]                ; = [this+0x88], same object as rdi
0x18015d439  mov rax,[rcx]
0x18015d43c  call [rax+0x368]
0x18015d456  ... epilogue, cookie check ...
0x18015d45d  ret
```

The two import calls resolve by name from the import table: `0x180532B60` is
`libcocos2d.dll!CCTouch::getLocation`, `0x180532B70` is
`libcocos2d.dll!CCNode::convertToNodeSpace`. So the body converts the touch position into
the slider node's space and hands the raw coordinate to one of two virtual slots, choosing
by `[slider+0x194]` (a vertical-slider flag), then refreshes the label/thumb through
`0x1801FF740` and a second virtual call at `+0x368`. What is missing compared with a normal
`ccTouchMoved` is exactly the usual min/max clamp and the mapping to a value range, which is
consistent with the changelog's "Slider Bypass" and with the label `HACK_SLIDER_LIMIT`.

UNVERIFIED: names of `SliderTouchLogic+0x88`/`+0x90`, of `[slider+0x194]` and of the two
virtual slots `+0xD0`/`+0xE0`/`+0x368`; whether the receiving setter re-clamps (in which case
the bound would be lifted by the *absence* of the pre-clamp only); and what `0x1801FF740`
does to `[this+0x90]`. The vanilla `ccTouchMoved` is host code and is not called here, so
its arithmetic cannot be compared inside this image.
