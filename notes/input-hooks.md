# Input hooks cluster

- **Question** — which GD input paths does Mega Hack hook, and what does each one do?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Filter `recovered/functions/_hook_sites.tsv` for GD methods whose names contain
   `Input`, `Button`, `Key`, `Touch`, `Mouse`, `Keyboard`, `Click` or `Press`.
2. Read the callback functions named in that table.
3. Cross-check each registrar and callback against `notes/host-methods.md` and
   `notes/hook-dispatch.md`.

## Evidence — input hook sites

| Registrar VA | GD method | Target | Callback VA | Callback kind | Helper |
| --- | --- | --- | --- | --- | --- |
| `0x18014ED70` | `cocos2d::CCEGLViewProtocol::handleTouchesBegin` | name-resolved | `0x18014FAC0` | function | `0x180150550` |
| `0x18014ED70` | `cocos2d::CCMouseDispatcher::dispatchScrollMSG` | name-resolved | `0x18014FB70` | function | `0x1801508C0` |
| `0x18014ED70` | `cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG` | name-resolved | `0x18014FBE0` | function | `0x180150B10` |
| `0x1801570E0` | `PlayerObject::pushButton` | RVA `0x397F40` | `0x180157180` | function | `0x1801580D0` |
| `0x18015D320` | `SliderTouchLogic::ccTouchMoved` | RVA `0x715D0` | `0x18015D390` | function | `0x18015D470` |
| `0x18015D6D0` | `CCTextInputNode::onTextFieldAttachWithIME` | RVA `0x508F0` | `0x18015D740` | function | `0x18015D770` |
| `0x180163560` | `GJScaleControl::ccTouchMoved` | RVA `0x12A770` | `0x180163A20` | function | `0x180163D60` |
| `0x180163F50` | `EditorOptionsLayer::onButtonRows` | RVA `0x291A00` | `0x180164020` | function | `0x180164340` |
| `0x180163F50` | `EditorOptionsLayer::onButtonsPerRow` | RVA `0x291970` | `0x1801641B0` | function | `0x180164340` |
| `0x180173FD0` | `UILayer::handleKeypress` | RVA `0x4CD830` | `0x1801740E0` | label | `0x1801742E0` |
| `0x180177B00` | `UILayer::handleKeypress` | RVA `0x4CD830` | `0x180177F80` | function | `0x1801742E0` |
| `0x1801812F0` | `PlayerObject::pushButton` | RVA `0x397F40` | `0x1801814B0` | function | `0x1801580D0` |
| `0x1801812F0` | `PlayerObject::releaseButton` | RVA `0x3981D0` | `0x180181500` | function | `0x1801580D0` |
| `0x180183CA0` | `PlayerObject::pushButton` | RVA `0x397F40` | `0x180183D70` | function | `0x1801580D0` |
| `0x180183CA0` | `PlayerObject::releaseButton` | RVA `0x3981D0` | `0x180183DC0` | function | `0x1801580D0` |
| `0x180184920` | `UILayer::handleKeypress` | RVA `0x4CD830` | `0x180184B10` | function | `0x1801742E0` |
| `0x180184920` | `PauseLayer::keyDown` | RVA `0x37ED10` | `0x180184B60` | function | `0x180185EA0` |
| `0x180187820` | `RewardUnlockLayer::showCloseButton` | RVA `0x3D0480` | `0x180187D20` | function | `0x180187FD0` |
| `0x180189260` | `MenuGameLayer::ccTouchBegan` | RVA `0x333600` | `0x180189400` | function | `0x180189630` |
| `0x180189A70` | `LevelAreaLayer::onClickDoor` | RVA `0x2C55C0` | `0x180189F90` | label | `0x18018A550` |
| `0x180190280` | `cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG` | name-resolved | `0x180190460` | function | `0x180150B10` |
| `0x180198BF0` | `GJBaseGameLayer::handleButton` | RVA `0x2338E0` | `0x180198D00` | function | `0x180198EF0` |
| `0x18019CA80` | `GJBaseGameLayer::handleButton` | RVA `0x2338E0` | `0x18019E4C0` | function | `0x180198EF0` |

## Evidence — what the callbacks do

- `PlayerObject::pushButton` hooks (`0x180157180`, `0x1801814B0`,
  `0x180183D70`) all share helper `FUN_1801580D0`. The first one checks whether the
  current level's `+0xDA0` / `+0xDA8` slots match the hooked `PlayLayer`/`PlayerObject`
  and, if so, calls `FUN_180156400` with mode `3` or `4`.
- `UILayer::handleKeypress` has two distinct registrars (`0x180173FD0` and
  `0x180177B00`) plus a third site (`0x180184920`), all converging on helper
  `FUN_1801742E0`, which builds a `TulipConvention` and calls the original through
  `FUN_180108E70`.
- `GJBaseGameLayer::handleButton` has two sites (`0x180198BF0`,
  `0x18019CA80`) sharing helper `FUN_180198EF0`; both target host RVA `0x2338E0`.
- The three input dispatchers (`handleTouchesBegin`, `dispatchScrollMSG`,
  `dispatchKeyboardMSG`) are name-resolved rather than RVA-resolved, and share helpers
  `FUN_180150550`, `FUN_1801508C0`, `FUN_180150B10`.

## Pattern

Input handling in Mega Hack is not one hook but a **family** of wrappers around the same
host method:

- `PlayerObject::pushButton` is registered three times (`0x1801570E0`,
  `0x1801812F0`, `0x180183CA0`) with one shared helper.
- `UILayer::handleKeypress` is registered three times (`0x180173FD0`,
  `0x180177B00`, `0x180184920`) with one shared helper.
- `GJBaseGameLayer::handleButton` is registered twice (`0x180198BF0`,
  `0x18019CA80`) with one shared helper.

That is the signature of a feature that needs to observe the same event from several
ownership contexts (player, UI layer, base game layer) rather than a single global
intercept.

## Open questions

- UNVERIFIED: which settings keys gate each input hook. The pushButton path reads
  `FUN_1801ACDB0` (current level) and `+0xDA0` / `+0xDA8`; the settings tags, if any,
  are not visible in the recovered callbacks shown here.
- UNVERIFIED: whether `PauseLayer::keyDown` and `MenuGameLayer::ccTouchBegan` are
  feature hooks or incidental registrations; their callback bodies are not yet read.
- UNVERIFIED: the exact behaviour of helper `FUN_180150B10` (keyboard dispatcher) and
  `FUN_180198EF0` (handleButton); only their call shape is recorded.

## Evidence — the keybinding system is `core::Keybinds` (2026-10-28)

**Question.** The v6.0.0 row "Added keybinding system" has no binding yet. Which class
implements it, how is a binding captured, and where is it stored?

**Method.** (1) `scripts/rtti_index.py --grep Keybinds` for the class and its base; (2) a
resolve of the complete object locator to the vtable and its slots, then `.pdata` extents via
`scripts/func_bounds.py`; (3) `scripts/build_callgraph.py` / `_callers.tsv` for the callers of
the singleton accessor and of `forward`; (4) `recovered/functions/_xrefs.tsv` and a raw-byte
scan for the settings paths, which are mostly SSO strings built with `movabs` and therefore
absent from a de-duplicated string list.

**Class identity.**

| Field | Value |
| --- | --- |
| RTTI name | `core::Keybinds` (`.?AVKeybinds@core@@` at `0x1806E1990`, TD `0x1806E1980`, COL `0x18053EC00`, class-hierarchy descriptor `0x18053EC18`) |
| Base | `hackpro::extensions::Delegate` (`_rtti_hierarchy.tsv`) |
| Vtable | `0x18053EBD8` (first function `0x18003E340`) |
| Singleton | object `0x180800C20`, accessor `0x18003D220` (magic static, `atexit` `0x18003D2F0`) |
| Storage | `keybinds.json` (`0x53EC5C`), read by the vtable slot `0x18003FC70` with `basic_ifstream` |

The mangled lambda names carry the two real member signatures even though the decompiler
shows them as `undefined`: `void core::Keybinds::forward(NXObject*)` and
`void (core::Keybinds::*)(hackpro::Tag, bool)` (`_rtti.tsv` `0x18053EA88`, `0x18053ED08`).

**Vtable slots** (`.pdata`, `func_bounds.py`):

| Slot | VA | Extent | What it reads / does |
| --- | --- | --- | --- |
| 0 | `0x18003E340` | `0x18003E340..0x18003EE59` | reads `KEYBINDS/DISABLE_IN_EDITOR` and `KEYBINDS/VERSION`; dispatches the `KEYBINDS/SET`, `/REMOVE` and `/VIEW` modes |
| 1 | `0x18003F860` | no `.pdata` record | leaf virtual with no stack, so it has no unwind entry; the recovered body is a bare `ret` |
| 2 | `0x18003F870` | `0x18003F870..0x18003FB43` | reads and writes the three mode flags `KEYBINDS/SET`, `KEYBINDS/VIEW`, `KEYBINDS/REMOVE` through `getConfigVar<bool>`/`setConfigVar<bool>` |
| 3 | `0x18003FC70` | `0x18003FC70..0x18003FEE6` | reads `keybinds.json` through `basic_ifstream` |

**Capture path — `forward` (`0x18003DD90..0x18003E1CB`).**

1. `FUN_1800DC300(NXObject*)` extracts the key description; if its `+0x10` is null the call
   returns (no key).
2. If `KEYBINDS/SET` is true: fetch `KEYBINDS/SHORTCUT` as `std::vector<unsigned int>`; an
   empty vector raises `MESSAGE/KEYBINDS/EMPTY` through `hackpro::extensions::showMessage`,
   otherwise the vector is copied and handed to `FUN_18003D7B0`.
3. Otherwise, if `KEYBINDS/REMOVE` is true: look the key up with `FUN_18007CB60`, erase the
   entry (`FUN_1800282A0`) and consume the event (`FUN_1800DDB50(obj, 0x539)`); a miss raises
   `MESSAGE/KEYBINDS/UNBINDABLE` through `showError`. `FUN_18003D7B0` raises the same message
   when the key is already bound elsewhere.

**Dispatch path.** `forward` is called by exactly three functions, `0x180022E90`,
`0x180022F90`, `0x180023080`, and each has the same shape: look the key up with
`FUN_18007CB60`, abort (`FUN_180009020`) when the key is not bound, take the singleton from
`0x18003D220`, and only call `forward` when the gate `FUN_18003DC20` returns true.
`0x18003DC20` is exactly `getConfigVar<bool>("KEYBINDS/SET") ||
getConfigVar<bool>("KEYBINDS/REMOVE")` — the capture-mode predicate — so the three callers are
the key-event entry points and `forward` runs only while the UI is waiting for a key. The
singleton is also reached from the mod bootstrap `0x180002320` and from `0x180044090`, which is
the only other caller of the bind routine `0x18003D7B0`.

**Settings paths** (raw bytes in `.rdata`; the `SET`, `REMOVE` and `VIEW` suffixes are
`movabs`-built):

| Path | Where |
| --- | --- |
| `KEYBINDS/VERSION`, `KEYBINDS/DISABLE_IN_EDITOR` | `.rdata` `0x53EBB8`, `0x53EB9D`; read in `0x18003E340` |
| `KEYBINDS/SHORTCUT` | `.rdata` `0x53EB74` |
| `KEYBINDS/SET`, `KEYBINDS/REMOVE`, `KEYBINDS/VIEW` | SSO strings in `0x18003E340` and `0x18003DD90` |
| `MESSAGE/KEYBINDS/EMPTY`, `MESSAGE/KEYBINDS/UNBINDABLE` | `.rdata` `0x53EB86`, `0x53EB58` |
| `ERROR/HOME/LEGACY_SAVE_KEYBINDS` | `.rdata` `0x53FB6F` |

**Conclusion.** The v6.0.0 "keybinding system" is the `core::Keybinds` singleton: a
`hackpro::extensions::Delegate` with a four-slot vtable, a `keybinds.json` store, a capture
mode (`KEYBINDS/SET` + `KEYBINDS/SHORTCUT`), a removal mode (`KEYBINDS/REMOVE`) and three
dispatch entry points. Two independent angles agree on the identity: RTTI plus vtable extents
from the image structures, and the settings paths plus control flow from the decompiled
bodies.

**Open questions.**

- UNVERIFIED: the concrete type behind `NXObject*` and what `FUN_1800DC300` reads at `+0x10`.
- UNVERIFIED: the storage format of `FUN_18007CB60` (the key -> tag map) and how
  `hackpro::Tag` reaches a feature; the `_Binder` lambda `void (Keybinds::*)(Tag, bool)`
  suggests a per-tag callback is registered with the extension host.
- UNVERIFIED: whether the three dispatch functions are hooks or callbacks; none appears in
  `_hook_sites.tsv`, and none has a `.pdata` neighbour named after a GD method.
- `ERROR/HOME/LEGACY_SAVE_KEYBINDS` implies keybinds were once saved inside the main save;
  the migration path was not read.
