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

1. `FUN_1800DC300(NXObject*)` is `return obj + 0x28`: it hands back the object's `std::string`
   (SSO layout, so the size sits at `+0x10` and the capacity at `+0x18` relative to that
   address). A zero length returns early.
2. If `KEYBINDS/SET` is true: fetch `KEYBINDS/SHORTCUT` as `std::vector<unsigned int>`; an
   empty vector raises `MESSAGE/KEYBINDS/EMPTY` through `hackpro::extensions::showMessage`,
   otherwise the vector is copied and handed to `FUN_18003D7B0`.
3. Otherwise, if `KEYBINDS/REMOVE` is true: look the name up in the Keybinds' own map at
   `this + 8`, replace its vector with an empty one (`FUN_1800282A0`) and consume the event
   (`FUN_1800DDB50(obj, 0x539)`); a miss raises `MESSAGE/KEYBINDS/UNBINDABLE` through
   `showError`. `FUN_18003D7B0` raises the same message when the name has no entry.

`FUN_18003D7B0` then writes the pending vector into the entry, clears `KEYBINDS/SET` and
empties `KEYBINDS/SHORTCUT`. `FUN_1800282A0` is a `std::vector` assign, not a bespoke routine:
its callers include the exported `setConfigVar<std::vector<unsigned int>>` (`0x180075310`), so
the stored value type is `std::vector<unsigned int>`.

The store is registry-shaped rather than a plain map: `FUN_18007CB60` reads its mask at `+0x88`,
sentinel at `+0x60` and buckets at `+0x70`, and `FUN_1800282A0` writes through the Tag-keyed
half at `+0xb0`/`+0xc0`/`+0xd8`. That is the same container pair and node layout that
[settings-registry.md](settings-registry.md) documents for the global registry, including the
`std::vector<unsigned int>` value at node `+0x18`/`+0x20` — so `core::Keybinds` owns a private
registry at `this + 8`, holding only the bindable names, rather than adding a member map.

**Where the name comes from.** The map is keyed by the object's name string, and the three
callers of `forward` show what kind of object that is. `0x180022E90`, `0x180022F90` and
`0x180023080` each resolve that same string through `FUN_18007CB60(p, &out, name)` with
`p = FUN_18008A510()`. `FUN_18008A510` is the accessor that `tagFor` (`0x180072DF0`) uses for
the registry and `FUN_18007CB60` is `tagFor`'s worker, the name -> `Tag` lookup
([settings-registry.md](settings-registry.md), "tagFor"). So the name is a **registered hack
name**, and the empty optional is treated as an invariant violation (`FUN_180009020`), not as a
normal miss. When capture mode is off the same three call
`FUN_18008B490(registry, tag, param_2)`, which is the "run this hack" branch.

`0x18003DC20` is exactly `getConfigVar<bool>("KEYBINDS/SET") ||
getConfigVar<bool>("KEYBINDS/REMOVE")` — the capture-mode predicate — so the sequence is:
resolve the row's hack name, and either capture a key for it (`forward`) or execute it. The
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
mode (`KEYBINDS/SET` + `KEYBINDS/SHORTCUT`), a removal mode (`KEYBINDS/REMOVE`) and three row
handlers that bind a key vector to a registered hack name or run the hack. Two independent
angles agree on the identity: RTTI plus vtable extents from the image structures, and the
settings paths plus control flow from the decompiled bodies.

**Open questions.**

- UNVERIFIED: what the third argument `param_2` of the three row handlers means (it reaches
  `FUN_18008B490` as the third argument), and whether the handlers fire per UI row or per key
  event; no caller exists in `_callers.tsv`, so they are entered indirectly.
- UNVERIFIED: who pre-populates the Keybinds map at `this + 8`. The lookup precedes any
  insert on the capture path, so an entry must exist before the first bind; the `VIEW` mode in
  `0x18003E340` and the `keybinds.json` reader are the two candidate sources, and neither was
  traced to an insert.
- UNVERIFIED: the path that turns a stored key vector into a hack execution. The three
  handlers run the hack when capture mode is off, but no read of the Keybinds map was found on
  that branch; the `_Binder` lambda `void (Keybinds::*)(Tag, bool)` is the likely bridge.
- `ERROR/HOME/LEGACY_SAVE_KEYBINDS` implies keybinds were once saved inside the main save;
  the migration path was not read.

## Evidence - tag 475 is the editor-state flag (2026-10-28)

The input initialiser `FUN_1800AFA10` (an `_initterm` entry, four statements of real work) is the
smallest of the registrar blocks, and it had no tag attribution until now. It does three things
in order:

1. `FUN_18014ED70()` - the three name-resolved input dispatcher hooks already recorded above
   (`CCEGLViewProtocol::handleTouchesBegin`, `CCMouseDispatcher::dispatchScrollMSG`,
   `CCKeyboardDispatcher::dispatchKeyboardMSG`).
2. `FUN_18008F910(registry, 0x1DB, &fn)` with `fn = FUN_180151280` - tag **475**.
3. `FUN_1800BA9D0()` - two more hooks, `MenuLayer::init` (RVA `0x333A90`) and host RVA `0x32ED60`.

A `mov edx,0x1db` capstone scan over `.text` finds fourteen sites; the only one with the
registration call shape (`mov rcx,rax; mov r8,rsi; call 0x18008F910`) is `0x1800AFA60`, inside
this initialiser. The other thirteen are unrelated immediates (string/UI code in the
`0x18023E2xx` range and `0x18003D781`).

**What tag 475 does.** `FUN_180151280(registry, bool *enabled)` installs four hooks at order
`0xFFFFFFE0` (-32) and then writes one flag:

| Target | RVA | Detour | Installer |
| --- | --- | --- | --- |
| `LevelEditorLayer::init` | `0x2D1DD0` | `FUN_180151420` | `FUN_180151640` |
| `EditorPauseLayer::onSaveAndPlay` | `0xDDEe0` | `FUN_180151440` | `FUN_180151890` |
| `EditorPauseLayer::onSaveAndExit` | `0xDE260` | `FUN_180151470` | `FUN_180151890` |
| `EditorPauseLayer::onExitNoSave` | `0xDE370` | `FUN_1801514A0` | `FUN_180151890` |

The detours are flag maintenance, not gameplay changes:

- `FUN_180151420` calls the original `init` and, only when it returned true, calls
  `FUN_1801514D0(0)`.
- `FUN_180151440` / `FUN_180151470` / `FUN_1801514A0` call `FUN_1801514D0(1)` and then the
  original save/exit method.

The final call in the callback is `FUN_1801514D0(~*enabled & 1 | (FUN_18014E990() == 0))`, so the
flag is also forced when the host field `[FUN_180147BA0() + 0x210]` reads zero.

**What the flag is.** `FUN_1801514D0(bool)` writes a boolean under the key `NO_EDITOR` inside the
`PARENT` sub-dictionary of the store returned by `FUN_1800D8E70()`: `FUN_1800DBE00(store,
"PARENT")` looks the sub-dictionary up (creating it if absent) and `FUN_1800DDD00(dict, value,
"NO_EDITOR")` stores the value. Both keys are SSO immediates, not `.rdata` strings - `PARENT` is
`0x544E45524150` and `NO_EDITOR` is `0x4F544944455F4F4E` plus the tail byte `0x52` - which is why
a string search for them finds nothing.

**Conclusion.** Tag 475 is not a gameplay toggle: it is the editor-state tracker that keeps a
`NO_EDITOR` boolean up to date so other parts of the mod can tell whether the editor is open
(true outside, false inside and during a save/exit). That is the shape a "disable something in
the editor" consumer needs, and it is why the block sits in the input initialiser next to the
three dispatcher hooks.

UNVERIFIED: which changelog row, if any, is tag 475. The bounded search is the settings tree,
which contains exactly two editor keys - `CREATOR/HACK_EDITOR_EXTENSION` (static label 50,
whose callback `FUN_18015F4F0` patches the host globals `0x623670`, `0x623674`, `0x623B04` and
`0x62368C`) and `CREATOR/HACK_SMOOTH_EDITOR_TRAIL` - and neither names an editor-state flag.
UNVERIFIED: who reads `PARENT/NO_EDITOR`; and whether `[host + 0x210]` is the host's
"editor open" field (the host image is not in `samples/`).
