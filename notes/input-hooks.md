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
