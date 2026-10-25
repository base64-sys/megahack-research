# Hook registration table: tag -> callback -> target

- **Question** — the initialiser `FUN_1800AFAE0` registers a callback per hack-flag tag.
  Which callback belongs to which tag, what is the tag's static label, and what does that
  callback hook, with which detour, name and installer?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.
- **Tools** — `scripts/tag_callback_map.py` (tag -> callback);
  `scripts/callback_hooks.py` (callback -> hooks); both dependency-free.
  [static-hack-label-map.md](static-hack-label-map.md) supplies tag -> `HACK_*` label.

## Answer

A full `.text` scan for the registration shape

```
mov edx, <tag>        ; hack-flag tag
mov rcx, rax          ; hash map (FNV-1a-64 of the name)
mov r8,  rsi          ; context
call 0x18008F910      ; bind callback
```

finds **125 registrations, 125 distinct tags**. **79** of them sit in the
first block, `0x1800AFAE0`–`0x1800B2000`, which earlier notes treated as the whole
initialiser; the function actually continues to `0x1800B3550`, and **46** further
registrations lie outside that block (listed below). **Correction (2026-10-25):** the count
of 79 published earlier the same day was window-bounded and undercounted by
46.

**79 of the 125 tags carry a `HACK_*` label** in the static map built by
`FUN_180062940`; the other **46 are runtime-assigned tags** that are not in
that table at all, which is why a name lookup for them fails instead of mismatching.

Reading each callback gives **186 same-image hooks across 111
callbacks**, plus **9 callbacks whose only hooks are resolved by module +
mangled export** (no RVA is available for those by construction),
**3 registration that resolves a callback but installs no hook**, and
**2 registration whose callback could not be located at all**. There are
**96 distinct installer helpers**, which settles a question earlier notes left
open: the installer is not one shared function but a per-callback thin wrapper (the five
documented in [render-hooks.md](render-hooks.md) are simply the ones met first).

## Columns

| Column | How it was obtained |
| --- | --- |
| Tag | **verified** — the `mov edx,imm32` that feeds the binder. |
| Static label | **verified** — the tag value appears in the 85-entry table in [static-hack-label-map.md](static-hack-label-map.md), each row anchored to its store VA. `—` is also evidence: the tag is not in that table. |
| Registration / callback | **verified** — `scripts/tag_callback_map.py`; the same tool reproduces all twenty pairs derived by hand in the per-feature notes. `—` in the callback column means no `lea rcx,[rip+disp]` was found within `0x180` bytes before the binder call. |
| Hook RVA | **verified as the resolver operand** — `scripts/callback_hooks.py` reads the `mov edx,imm32` immediately preceding a `call 0x18010A370`. That is the value handed to the export that resolves an image function, so it is the resolution key, not a guess. |
| Detour / installer | **verified as operands** — the `lea r8,[rip+disp]` and the `call` that follows the name operand, both read from the instruction bytes. |
| Friendly name | **read from `.rdata`** at the `lea r9,[rip+disp]` target. It is a **logging label**: the same hook can be resolved by RVA while its name string differs, so the name is evidence about intent, not about which function was hooked. |

What the table does **not** establish for any row: the detour's own behaviour (veto,
observe, rewrite) or the effect of the order constant. Those are per-feature notes' work.

## The table

| Tag | Dec | Static label | Registration | Callback | Hook(s): target &middot; name &middot; detour &middot; installer |
| --- | ---: | --- | --- | --- | --- |
| `0x020` | 32 | `HACK_UNLOCK_ICONS` | `0x1800AFF01` | `0x18015DC00` | `0x17C500` GameManager::isIconUnlocked &middot; detour `0x18015DCD0` &middot; installer `0x18015DCF0`<br>`0x17C8E0` GameManager::isColorUnlocked &middot; detour `0x18015DCE0` &middot; installer `0x18015DE40` |
| `0x021` | 33 | `HACK_UNLOCK_VAULTS` | `0x1800AFFEF` | `0x18015E220` | `0x9E590` CreatorLayer::onSecretVault &middot; detour `0x18015E2F0` &middot; installer `0x18015DA10`<br>`0x372CD0` OptionsLayer::onSecretVault &middot; detour `0x18015E330` &middot; installer `0x18015E370` |
| `0x022` | 34 | `HACK_UNLOCK_SHOPS` | `0x1800AFF78` | `0x18015DFA0` | `0x3D66D0` SecretRewardsLayer::onShop &middot; detour `0x18015E010` &middot; installer `0x18015E030` |
| `0x023` | 35 | `HACK_TREASURE_ROOM` | `0x1800AFE8A` | `0x18015D960` | `0x9EDA0` CreatorLayer::onTreasureRoom &middot; detour `0x18015D9D0` &middot; installer `0x18015DA10` |
| `0x024` | 36 | `HACK_MUSIC_UNLOCKER` | `0x1800AFD25` | `0x18015D130` | `0x1E5820` GameStatsManager::isItemUnlocked &middot; detour `0x18015D1A0` &middot; installer `0x18015D1C0` |
| `0x025` | 37 | `HACK_TEXT_LENGTH` | `0x1800AFE13` | `0x18015D6D0` | `0x508F0` CCTextInputNode::onTextFieldAttachWithIME &middot; detour `0x18015D740` &middot; installer `0x18015D770` |
| `0x026` | 38 | `HACK_SLIDER_LIMIT` | `0x1800AFD9C` | `0x18015D320` | `0x715D0` SliderTouchLogic::ccTouchMoved &middot; detour `0x18015D390` &middot; installer `0x18015D470` |
| `0x027` | 39 | `HACK_MAIN_LEVELS` | `0x1800AFCB1` | `0x18015C9E0` | `0x31D880` LevelPage::onPlay &middot; detour `0x18015CB10` &middot; installer `0x18015CB80`<br>`0x31A8B0` LevelPage::updateDynamicPage &middot; detour `0x18015CB30` &middot; installer `0x18015CD60`<br>`0x1D51B0` GameStatsManager::getStat &middot; detour `0x18015CB50` &middot; installer `0x18015CF40` |
| `0x028` | 40 | — | `0x1800AFB88` | `0x18015C3F0` | `0x20B050` GJBaseGameLayer::shouldExitHackedLevel &middot; detour `0x18015C460` &middot; installer `0x18015B120` |
| `0x029` | 41 | — | `0x1800AFBEA` | `0x18015C480` | `0x3E4910` SecretLayer2::onDoor &middot; detour `0x18015C4F0` &middot; installer `0x18015C530` |
| `0x02A` | 42 | — | `0x1800AFC4C` | `0x18015C720` | `0x3E22E0` SecretLayer2::init &middot; detour `0x18015C7F0` &middot; installer `0x180154610`<br>`0x3E4660` SecretLayer2::onSecretLevel &middot; detour `0x18015C910` &middot; installer `0x18015C530` |
| `0x02B` | 43 | `HACK_COPY_HACK` | `0x1800B00DD` | `0x18015E920` | `0x2F7340` LevelInfoLayer::init &middot; detour `0x18015E9F0` &middot; installer `0x18015EAB0`<br>`0x2FBA60` LevelInfoLayer::levelDownloadFinished &middot; detour `0x18015EA50` &middot; installer `0x18015ED00` |
| `0x02C` | 44 | `HACK_NO_C_MARK` | `0x1800B049A` | `0x180162D20` | `0x490270` ShareLevelLayer::onShare &middot; detour `0x180162D90` &middot; installer `0x180162E10` |
| `0x02D` | 45 | `HACK_LEVEL_EDIT` | `0x1800B03A7` | `0x18015FD40` | `0x37C870` PauseLayer::customSetup &middot; detour `0x18015FE70` &middot; installer `0x1800AA5D0`<br>`0x37E550` PauseLayer::onTryEdit &middot; detour `0x18015FEC0` &middot; installer `0x1800AA770`<br>`0xD9EC0` EditorPauseLayer::init &middot; detour `0x18015FF20` &middot; installer `0x18015FF70` |
| `0x02E` | 46 | `HACK_CUSTOM_OBJECT_BYPASS` | `0x1800B0154` | `0x18015EEF0` | `0xE4030` EditorUI::onNewCustomItem &middot; detour `0x18015EF60` &middot; installer `0x18015F000` |
| `0x02F` | 47 | `HACK_TOOLBOX_BUTTON_BYPASS` | `0x1800B0604` | `0x180163F50` | `0x291A00` EditorOptionsLayer::onButtonRows &middot; detour `0x180164020` &middot; installer `0x180164340`<br>`0x291970` EditorOptionsLayer::onButtonsPerRow &middot; detour `0x1801641B0` &middot; installer `0x180164340` |
| `0x030` | 48 | `HACK_VERIFY_HACK` | `0x1800B06F2` | `0x180164F00` | `0x490270` ShareLevelLayer::onShare &middot; detour `0x180164F70` &middot; installer `0x180162E10` |
| `0x031` | 49 | `HACK_DEFAULT_SONG_BYPASS` | `0x1800B01CB` | `0x18015F1F0` | `0xC7810` SongSelectNode::audioPrevious &middot; detour `0x18015F2C0` &middot; installer `0x18015F300`<br>`0xC77D0` SongSelectNode::audioNext &middot; detour `0x18015F2E0` &middot; installer `0x18015F300` |
| `0x032` | 50 | `HACK_EDITOR_EXTENSION` | `0x1800B0242` | `0x18015F4F0` | **no hook install** |
| `0x033` | 51 | `HACK_PLACE_OVER` | `0x1800B0511` | `0x180163000` | `0x2D4A50` LevelEditorLayer::typeExistsAtPosition &middot; detour `0x180163070` &middot; installer `0x180163080` |
| `0x034` | 52 | `HACK_FREE_SCROLL` | `0x1800B02B9` | `0x18015F890` | `0x122980` EditorUI::constrainGameLayerPosition &middot; detour `0x18015F900` &middot; installer `0x18015F910` |
| `0x035` | 53 | `HACK_HIDE_UI` | `0x1800B0330` | `0x18015FA70` | `0xDF390` EditorUI::init &middot; detour `0x18015FB20` &middot; installer `0x18015FB50` |
| `0x036` | 54 | `HACK_TRIGGER_VALUE_BYPASS` | `0x1800B067B` | `0x180164530` | `0x475FD0` SetupTriggerPopup::createValueControlAdvanced &middot; detour `0x1801645A0` &middot; installer `0x1801647E0` |
| `0x037` | 55 | `HACK_SMOOTH_EDITOR_TRAIL` | `0x1800B058D` | `0x180163EC0` | `0x2E0F40` LevelEditorLayer::postUpdate &middot; detour `0x180163F30` &middot; installer `0x180158CC0` |
| `0x038` | 56 | — | `0x1800B0066` | `0x18015E560` | `0xDD840` EditorPauseLayer::saveLevel &middot; detour `0x18015E5D0` &middot; installer `0x18015E770` |
| `0x03D` | 61 | — | `0x1800B0423` | `0x1801607F0` | `0x2DFCD0` LevelEditorLayer::onPlaytest &middot; detour `0x1801608C0` &middot; installer `0x1800ABBD0`<br>`0x2E6C70` LevelEditorLayer::updateDebugDraw &middot; detour `0x180160FA0` &middot; installer `0x1800ABBD0` |
| `0x048` | 72 | `HACK_NOCLIP` | `0x1800B2723` | `0x18017A220` | `0x3B39D0` PlayLayer::destroyPlayer &middot; detour `0x18017A290` &middot; installer `0x180169480` |
| `0x058` | 88 | `HACK_PRACTICE_MUSIC` | `0x1800B2904` | `0x18017DE30` | `0x1E5820` GameStatsManager::isItemUnlocked &middot; detour `0x18017E000` &middot; installer `0x18015D1C0` |
| `0x059` | 89 | `HACK_INSTANT_COMPLETE` | `0x1800B25B9` | `0x1801789E0` | `0x3B4BA0` PlayLayer::postUpdate &middot; detour `0x180178B10` &middot; installer `0x1800AB9F0`<br>`0x3A7A80` PlayLayer::levelComplete &middot; detour `0x180178BC0` &middot; installer `0x1800AB850`<br>`0x3AB9A0` PlayLayer::updateTimeWarp &middot; detour `0x180178BE0` &middot; installer `0x1800AB9F0` |
| `0x082` | 130 | `HACK_STARTPOS_SWITCHER` | `0x1800B2E30` | `0x180184920` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180184A80` &middot; installer `0x1800AB850`<br>`0x4CD830` UILayer::handleKeypress &middot; detour `0x180184B10` &middot; installer `0x1801742E0`<br>`0x37ED10` PauseLayer::keyDown &middot; detour `0x180184B60` &middot; installer `0x180185EA0` |
| `0x0E0` | 224 | `HACK_SHOW_HITBOXES_TRAIL` | `0x1800B2BD3` | `0x1801801D0` | `0x239C60` GJBaseGameLayer::processCommands &middot; detour `0x180180240` &middot; installer `0x180174920` |
| `0x0EB` | 235 | `HACK_SMART_STARTPOS` | `0x1800B2DB4` | `0x180183E30` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180183EA0` &middot; installer `0x1800AB850` |
| `0x0F1` | 241 | `HACK_ALL_MODES_PLATFORMER` | `0x1800B1CDA` | `0x180174590` | `0x214960` GJBaseGameLayer::collisionCheckObjects &middot; detour `0x180174600` &middot; installer `0x180167330` |
| `0x0F2` | 242 | `HACK_FORCE_PLATFORMER` | `0x1800B24CB` | `0x180178710` | `0x23AD00` GJBaseGameLayer::loadLevelSettings &middot; detour `0x180178780` &middot; installer `0x180172C30` |
| `0x0F5` | 245 | `HACK_RESPAWN_TIME` | `0x1800B2AE5` | `0x18017EF50` | `0x3B39D0` PlayLayer::destroyPlayer &middot; detour `0x18017EFC0` &middot; installer `0x180169480` |
| `0x104` | 260 | `HACK_SHOW_LAYOUT` | `0x1800B2B5C` | `0x18017F0F0` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x18017F330` &middot; installer `0x1800AB850`<br>`0x3B1660` PlayLayer::applyEnterEffect &middot; detour `0x18017F750` &middot; installer `0x18017FAA0`<br>`0x3B0970` PlayLayer::applyCustomEnterEffect &middot; detour `0x18017F7C0` &middot; installer `0x18017FD40`<br>`0x4A5F30` EffectGameObject::triggerObject &middot; detour `0x18017F830` &middot; installer `0x180168350`<br>`0x18DEE0` GameObject::addGlow &middot; detour `0x18017F8E0` &middot; installer `0x18017FF90`<br>`0x1A08D0` GameObject::saveActiveColors &middot; detour `0x18017FA60` &middot; installer `0x18017EB00` |
| `0x117` | 279 | `HACK_AUTO_DEAFEN` | `0x1800B1DC8` | `0x180174A80` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180174C80` &middot; installer `0x1800AB850`<br>`0x3B39D0` PlayLayer::destroyPlayer &middot; detour `0x180174CA0` &middot; installer `0x180169480`<br>`0x3B4BA0` PlayLayer::postUpdate &middot; detour `0x180174CD0` &middot; installer `0x1800AB9F0`<br>`0x3BA0E0` PlayLayer::pauseGame &middot; detour `0x180174DB0` &middot; installer `0x180174EC0`<br>`0x3BABB0` PlayLayer::onQuit &middot; detour `0x180174DE0` &middot; installer `0x1800AB850` |
| `0x11E` | 286 | `HACK_SHOW_TRIGGERS` | `0x1800B2CC6` | `0x180183790` | `0x3ADD70` PlayLayer::addObject &middot; detour `0x1801838C0` &middot; installer `0x18015B2C0`<br>`0x226500` GJBaseGameLayer::addToSection &middot; detour `0x180183910` &middot; installer `0x180183AB0`<br>`0x190E20` GameObject::customSetup &middot; detour `0x180183920` &middot; installer `0x18017EB00` |
| `0x123` | 291 | `HACK_JUMPSCARE` | `0x1800B26A7` | `0x180178D10` | `0x3B39D0` PlayLayer::destroyPlayer &middot; detour `0x180178D80` &middot; installer `0x180169480` |
| `0x128` | 296 | `HACK_0_PRACTICE_COMPLETE` | `0x1800B2980` | `0x18017E0B0` | `0x3ABA00` PlayLayer::playEndAnimationToPos &middot; detour `0x18017E180` &middot; installer `0x18015A300`<br>`0x3AC3E0` PlayLayer::playPlatformerEndAnimationToPos &middot; detour `0x18017E210` &middot; installer `0x18015A4E0` |
| `0x129` | 297 | `HACK_COLLECT_COINS_IN_PRACTICE` | `0x1800B2185` | `0x180176EA0` | `0x214960` GJBaseGameLayer::collisionCheckObjects &middot; detour `0x180176F10` &middot; installer `0x180167330` |
| `0x12A` | 298 | `HACK_CHECKPOINT_LIMIT_BYPASS` | `0x1800B2092` | `0x180176250` | `0x3B74A0` PlayLayer::storeCheckpoint &middot; detour `0x1801762C0` &middot; installer `0x180176300` |
| `0x12E` | 302 | `HACK_FRAME_STEPPER` | `0x1800B23DD` | `0x180177B00` | `0x237850` GJBaseGameLayer::update &middot; detour `0x180177D90` &middot; installer `0x180172EC0`<br>`0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180177E00` &middot; installer `0x1800AB850`<br>`0x3B4BA0` PlayLayer::postUpdate &middot; detour `0x180177E90` &middot; installer `0x1800AB9F0`<br>`0x397480` PlayerObject::playSpawnEffect &middot; detour `0x180177F70` &middot; installer `0x1800AB230`<br>`0x4CD830` UILayer::handleKeypress &middot; detour `0x180177F80` &middot; installer `0x1801742E0` |
| `0x134` | 308 | `HACK_RANDOM_SEED` | `0x1800B29F7` | `0x18017E2C0` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x18017E6A0` &middot; installer `0x1800AB850` |
| `0x137` | 311 | `HACK_CONFIRM_PRACTICE` | `0x1800B2273` | `0x1801772E0` | `0x37E1C0` PauseLayer::onPracticeMode &middot; detour `0x180177350` &middot; installer `0x1800AA770` |
| `0x138` | 312 | `HACK_CONFIRM_NORMAL` | `0x1800B21FC` | `0x180176FF0` | `0x37E2A0` PauseLayer::onNormalMode &middot; detour `0x180177060` &middot; installer `0x1800AA770` |
| `0x139` | 313 | `HACK_CONFIRM_RESET` | `0x1800B22EA` | `0x1801774B0` | `0x37E410` PauseLayer::onRestart &middot; detour `0x180177520` &middot; installer `0x1800AA770` |
| `0x13A` | 314 | `HACK_CONFIRM_FULL_RESET` | `0x1800B2361` | `0x180177680` | `0x37E4B0` PauseLayer::onRestartFull &middot; detour `0x1801776F0` &middot; installer `0x1800AA770` |
| `0x13C` | 316 | `HACK_REPLAY_LAST_CHECKPOINT` | `0x1800B2A6E` | `0x18017ECB0` | `0x134E80` EndLevelLayer::onReplay &middot; detour `0x18017ED20` &middot; installer `0x18017ED60` |
| `0x13D` | 317 | `HACK_PRACTICE_BUG_FIX` | `0x1800B288D` | `0x18017B620` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x18017B690` &middot; installer `0x1800AB850` |
| `0x140` | 320 | `HACK_AUTO_PICKUP_COINS` | `0x1800B1F2D` | `0x1801754C0` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180175530` &middot; installer `0x1800AB850` |
| `0x143` | 323 | `HACK_AUTO_SONG_DOWNLOAD` | `0x1800B1FA4` | `0x180175640` | `0x2F7340` LevelInfoLayer::init &middot; detour `0x1801756B0` &middot; installer `0x18015EAB0` |
| `0x146` | 326 | `HACK_AUTO_MUSIC_SYNC` | `0x1800B1EB6` | `0x1801751E0` | `0x3B4BA0` PlayLayer::postUpdate &middot; detour `0x180175250` &middot; installer `0x1800AB9F0` |
| `0x14B` | 331 | — | `0x1800B1E3F` | `0x1801750B0` | `0x3B4BA0` PlayLayer::postUpdate &middot; detour `0x180175120` &middot; installer `0x1800AB9F0` |
| `0x14E` | 334 | — | `0x1800B2630` | `0x180178C60` | `0x237850` GJBaseGameLayer::update &middot; detour `0x180178CD0` &middot; installer `0x180172EC0` |
| `0x155` | 341 | — | `0x1800B2542` | `0x1801787C0` | `0x3A5B00` PlayLayer::init &middot; detour `0x180178880` &middot; installer `0x1800AB5B0`<br>`0x3ADD70` PlayLayer::addObject &middot; detour `0x1801788C0` &middot; installer `0x18015B2C0` |
| `0x160` | 352 | — | `0x1800B279A` | `0x18017A710` | `0x214960` GJBaseGameLayer::collisionCheckObjects &middot; detour `0x18017A780` &middot; installer `0x180167330` |
| `0x168` | 360 | — | `0x1800B2C4F` | `0x180181540` | `0x3B4BA0` PlayLayer::postUpdate &middot; detour `0x180181780` &middot; installer `0x1800AB9F0`<br>`0x3B39D0` PlayLayer::destroyPlayer &middot; detour `0x180181880` &middot; installer `0x180169480`<br>`0x2E0F40` LevelEditorLayer::postUpdate &middot; detour `0x1801818E0` &middot; installer `0x180158CC0`<br>`0x2E0630` LevelEditorLayer::onStopPlaytest &middot; detour `0x180181A00` &middot; installer `0x1800ABBD0`<br>`0x214960` GJBaseGameLayer::collisionCheckObjects &middot; detour `0x180181AA0` &middot; installer `0x180167330`<br>`0x38CBA0` PlayerObject::incrementJumps &middot; detour `0x180181C70` &middot; installer `0x1800AB230` |
| `0x16C` | 364 | — | `0x1800B2454` | `0x180178650` | `0x3ADD70` PlayLayer::addObject &middot; detour `0x1801786C0` &middot; installer `0x18015B2C0` |
| `0x16E` | 366 | — | `0x1800B2D3D` | `0x180183CA0` | `0x397F40` PlayerObject::pushButton &middot; detour `0x180183D70` &middot; installer `0x1801580D0`<br>`0x3981D0` PlayerObject::releaseButton &middot; detour `0x180183DC0` &middot; installer `0x1801580D0` |
| `0x170` | 368 | — | `0x1800B210E` | `0x1801768A0` | **no hook install** |
| `0x174` | 372 | — | `0x1800B2811` | `0x18017B430` | `0x3BA0E0` PlayLayer::pauseGame &middot; detour `0x18017B500` &middot; installer `0x180174EC0`<br>`0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x18017B530` &middot; installer `0x1800AB850` |
| `0x175` | 373 | — | `0x1800B201B` | `0x180175730` | `0x3A5B00` PlayLayer::init &middot; detour `0x180175800` &middot; installer `0x1800AB5B0`<br>`0x2FD310` LevelInfoLayer::playStep4 &middot; detour `0x180175830` &middot; installer `0x1801760A0` |
| `0x17B` | 379 | — | `0x1800B1D51` | `0x1801746F0` | `0x239C60` GJBaseGameLayer::processCommands &middot; detour `0x180174760` &middot; installer `0x180174920` |
| `0x181` | 385 | `HACK_HIDE_PAUSE_BUTTON` | `0x1800B0B26` | `0x180167D10` | `0x4CCB00` UILayer::init &middot; detour `0x180167D80` &middot; installer `0x180167DB0` |
| `0x182` | 386 | `HACK_HIDE_PLAYER` | `0x1800B0AAF` | `0x180167A60` | `0x38B440` PlayerObject::toggleVisibility &middot; detour `0x180167B10` &middot; installer `0x180167B20` |
| `0x183` | 387 | `HACK_NO_CAMERA` | `0x1800B0C14` | `0x1801680E0` | `0x4BD820` CameraTriggerGameObject::triggerObject &middot; detour `0x180168150` &middot; installer `0x180168160` |
| `0x184` | 388 | `HACK_NO_CAMERA_ZOOM` | `0x1800B0C8B` | `0x1801682C0` | `0x4A5F30` EffectGameObject::triggerObject &middot; detour `0x180168330` &middot; installer `0x180168350` |
| `0x185` | 389 | `HACK_NO_SHADERS` | `0x1800B14E9` | `0x180171620` | `0x4880B0` ShaderLayer::performCalculations &middot; detour `0x1801716F0` &middot; installer `0x180171710`<br>`0x488A60` ShaderLayer::visit &middot; detour `0x180171700` &middot; installer `0x180171710` |
| `0x186` | 390 | `HACK_CLASSIC_PARTICLES` | `0x1800B085C` | `0x180166270` | symbol-resolved (module + mangled export) |
| `0x188` | 392 | `HACK_TRAIL_CUTTING` | `0x1800B1BE2` | `0x1801732C0` | symbol-resolved (module + mangled export) |
| `0x18A` | 394 | `HACK_NO_WAVE_PULSE` | `0x1800B173C` | `0x180172160` | `0x388D80` PlayerObject::update &middot; detour `0x1801721D0` &middot; installer `0x1800AAC20` |
| `0x18B` | 395 | `HACK_NO_TRAIL` | `0x1800B16C5` | `0x180171DB0` | symbol-resolved (module + mangled export) |
| `0x18C` | 396 | `HACK_SOLID_WAVE_TRAIL` | `0x1800B1A06` | `0x1801728C0` | `0x3887C0` PlayerObject::setupStreak &middot; detour `0x180172A80` &middot; installer `0x1800AB230`<br>`0x388CA0` PlayerObject::updateStreakBlend &middot; detour `0x180172B00` &middot; installer `0x180167B20` |
| `0x18D` | 397 | `HACK_NO_WAVE_TRAIL` | `0x1800B17B3` | `0x1801721F0` | `0x2B8EF0` HardStreak::updateStroke &middot; detour `0x180172260` &middot; installer `0x180167020` |
| `0x18E` | 398 | `HACK_NO_RESPAWN_FLASH` | `0x1800B13FB` | `0x1801714C0` | `0x397480` PlayerObject::playSpawnEffect &middot; detour `0x180171530` &middot; installer `0x1800AB230` |
| `0x18F` | 399 | `HACK_NO_DEATH_EFFECT` | `0x1800B0DF0` | `0x180169330` | `0x37EF10` PlayerObject::playDeathEffect &middot; detour `0x180169400` &middot; installer `0x1800AB230`<br>`0x3B39D0` PlayLayer::destroyPlayer &middot; detour `0x180169410` &middot; installer `0x180169480` |
| `0x192` | 402 | `HACK_NO_PARTICLES` | `0x1800B11A8` | `0x1801703B0` | symbol-resolved (module + mangled export) |
| `0x195` | 405 | — | `0x1800B121F` | `0x180170DC0` | symbol-resolved (module + mangled export) |
| `0x197` | 407 | `HACK_WAVE_PULSE_SIZE` | `0x1800B1C59` | `0x180173920` | `0x388D80` PlayerObject::update &middot; detour `0x180173990` &middot; installer `0x1800AAC20` |
| `0x199` | 409 | `HACK_NO_SHAKE` | `0x1800B1560` | `0x1801718C0` | `0x3A5B00` PlayLayer::init &middot; detour `0x180171930` &middot; installer `0x1800AB5B0` |
| `0x19A` | 410 | `HACK_HIDE_PAUSE_MENU` | `0x1800B0B9D` | `0x180167FA0` | `0x37C870` PauseLayer::customSetup &middot; detour `0x1801680A0` &middot; installer `0x1800AA5D0` |
| `0x19B` | 411 | `HACK_WAVE_TRAIL_FIX` | `0x1800B094A` | `0x180166A80` | `0x2B8EF0` HardStreak::updateStroke &middot; detour `0x180166AF0` &middot; installer `0x180167020` |
| `0x19C` | 412 | `HACK_NO_DO_NOT_FLIP` | `0x1800B0E67` | `0x1801696E0` | `0x132E50` EndLevelLayer::customSetup &middot; detour `0x1801697B0` &middot; installer `0x180169890`<br>`0x3A66D0` PlayLayer::setupHasCompleted &middot; detour `0x180169820` &middot; installer `0x1800AB850` |
| `0x19D` | 413 | `HACK_NO_PULSE` | `0x1800B1384` | `0x1801713E0` | `0x3AF3A0` PlayLayer::updateVisibility &middot; detour `0x180171450` &middot; installer `0x1800AB9F0` |
| `0x19E` | 414 | `HACK_NO_WAVE_TRAIL_BEHIND` | `0x1800B182A` | `0x180172280` | `0x3A0090` PlayerObject::activateStreak &middot; detour `0x1801722F0` &middot; installer `0x1800AB230` |
| `0x19F` | 415 | `HACK_STOP_TRIGGERS_ON_DEATH` | `0x1800B1AF4` | `0x180172DE0` | `0x237850` GJBaseGameLayer::update &middot; detour `0x180172E50` &middot; installer `0x180172EC0` |
| `0x1A0` | 416 | `HACK_NO_PORTAL_LIGHTNING` | `0x1800B1296` | `0x180171170` | `0x246D00` GJBaseGameLayer::lightningFlash &middot; detour `0x1801711E0` &middot; installer `0x1801711F0` |
| `0x1A1` | 417 | — | `0x1800B1043` | `0x180169C00` | `0x2467D0` GJBaseGameLayer::toggleFlipped &middot; detour `0x180169C70` &middot; installer `0x180169C80` |
| `0x1A2` | 418 | — | `0x1800B0FCC` | `0x180169B70` | `0x3ADD70` PlayLayer::addObject &middot; detour `0x180169BE0` &middot; installer `0x18015B2C0` |
| `0x1A4` | 420 | — | `0x1800B1131` | `0x18016A370` | `0x4A1DB0` RingObject::spawnCircle &middot; detour `0x18016A3E0` &middot; installer `0x18016A3F0` |
| `0x1A5` | 421 | — | `0x1800B130D` | `0x180171350` | `0x397760` PlayerObject::spawnPortalCircle &middot; detour `0x1801713C0` &middot; installer `0x180168B00` |
| `0x1A6` | 422 | — | `0x1800B18A1` | `0x180172330` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180172430` &middot; installer `0x1800AB850` |
| `0x1A7` | 423 | — | `0x1800B07E5` | `0x180165890` | `0x39BE30` PlayerObject::switchedToMode &middot; detour `0x180165A20` &middot; installer `0x180165BE0`<br>`0x398C00` PlayerObject::ringJump &middot; detour `0x180165B60` &middot; installer `0x180165DC0`<br>`0x38D480` PlayerObject::runBallRotation2 &middot; detour `0x180165B80` &middot; installer `0x1800AB230`<br>`0x39BF30` PlayerObject::hitGround &middot; detour `0x180165BC0` &middot; installer `0x180166010` |
| `0x1A8` | 424 | — | `0x1800B198F` | `0x1801726B0` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180172720` &middot; installer `0x1800AB850` |
| `0x1A9` | 425 | — | `0x1800B08D3` | `0x180166660` | `0x553C0` FMODAudioEngine::update &middot; detour `0x1801666D0` &middot; installer `0x180166890` |
| `0x1AA` | 426 | `HACK_STARTPOS_RESET_CAMERA` | `0x1800B1A7D` | `0x180172B90` | `0x235760` GJBaseGameLayer::loadStartPosObject &middot; detour `0x180172C00` &middot; installer `0x180172C30` |
| `0x1AD` | 429 | — | `0x1800B076E` | `0x180165040` | `0x3B23C0` PlayLayer::updateProgressbar &middot; detour `0x180165110` &middot; installer `0x1800AB850`<br>`0x3B3950` PlayLayer::getCurrentPercent &middot; detour `0x1801655E0` &middot; installer `0x1801656E0` |
| `0x1B0` | 432 | — | `0x1800B0F55` | `0x180169AE0` | `0x39EE30` PlayerObject::toggleGhostEffect &middot; detour `0x180169B50` &middot; installer `0x1800AB3D0` |
| `0x1B1` | 433 | — | `0x1800B0A38` | `0x180167490` | `0x40CF0` CCAnimatedSprite::runAnimation &middot; detour `0x180167560` &middot; installer `0x180167620`<br>`0x40D60` CCAnimatedSprite::tweenToAnimation &middot; detour `0x1801675C0` &middot; installer `0x180167800` |
| `0x1B2` | 434 | — | `0x1800B15D7` | `0x1801719C0` | `0x395170` PlayerObject::playSpiderDashEffect &middot; detour `0x180171A30` &middot; installer `0x180171A40` |
| `0x1B3` | 435 | — | `0x1800B10BA` | `0x180169DE0` | `0x3A9550` PlayLayer::showNewBest &middot; detour `0x180169EB0` &middot; installer `0x180169FA0`<br>`0x3B39D0` PlayLayer::destroyPlayer &middot; detour `0x180169F20` &middot; installer `0x180169480` |
| `0x1B4` | 436 | — | `0x1800B0D79` | `0x180169260` | `0x396D20` PlayerObject::updateDashAnimation &middot; detour `0x1801692D0` &middot; installer `0x1800AB230` |
| `0x1B5` | 437 | — | `0x1800B0D02` | `0x1801684B0` | `0x39FA50` PlayerObject::playBumpEffect &middot; detour `0x1801687B0` &middot; installer `0x1801688B0`<br>`0x398C00` PlayerObject::ringJump &middot; detour `0x1801687D0` &middot; installer `0x180165DC0`<br>`0x397760` PlayerObject::spawnPortalCircle &middot; detour `0x1801687F0` &middot; installer `0x180168B00`<br>`0x397B70` PlayerObject::spawnDualCircle &middot; detour `0x180168800` &middot; installer `0x1800AB230`<br>`0x39AF90` PlayerObject::toggleDartMode &middot; detour `0x180168810` &middot; installer `0x1800AAFE0`<br>`0x3A0700` PlayerObject::togglePlayerScale &middot; detour `0x180168830` &middot; installer `0x1800AAFE0`<br>`0x1AA4A0` GameObject::playDestroyObjectAnim &middot; detour `0x180168850` &middot; installer `0x180168D50`<br>`0x42870` CCCircleWave::create &middot; detour `0x180168870` &middot; installer `0x180168F30` |
| `0x1B6` | 438 | — | `0x1800B09C1` | `0x180167210` | `0x214960` GJBaseGameLayer::collisionCheckObjects &middot; detour `0x180167280` &middot; installer `0x180167330` |
| `0x1B7` | 439 | — | `0x1800B0EDE` | `0x180169A40` | `0x3A8F30` PlayLayer::showCompleteEffect &middot; detour `0x180169AB0` &middot; installer `0x1800AB850` |
| `0x1B8` | 440 | — | `0x1800B1472` | `0x180171550` | `0x39B6F0` PlayerObject::toggleRobotMode &middot; detour `0x1801715C0` &middot; installer `0x1800AAFE0` |
| `0x1B9` | 441 | — | `0x1800B164E` | `0x180171CA0` | `0x39AB20` PlayerObject::toggleSwingMode &middot; detour `0x180171D10` &middot; installer `0x1800AAFE0` |
| `0x1BA` | 442 | — | `0x1800B1918` | `0x1801724B0` | `0x3B8EB0` PlayLayer::resetLevel &middot; detour `0x180172540` &middot; installer `0x1800AB850` |
| `0x1BD` | 445 | — | `0x1800A8206` | — | **callback unresolved** (no `lea rcx,[rip+disp]` within 0x180 bytes) |
| `0x1C0` | 448 | — | `0x1800B1B6B` | `0x1801730B0` | symbol-resolved (module + mangled export) |
| `0x1C1` | 449 | `HACK_NO_TRANSITION` | `0x1800B317B` | `0x180189A70` | symbol-resolved (module + mangled export)<br>symbol-resolved (module + mangled export)<br>`0x2ACD70` GJShopLayer::onBack &middot; detour `0x180189F70` &middot; installer `0x18018A370`<br>`0x2C55C0` LevelAreaLayer::onClickDoor &middot; detour `0x180189F90` &middot; installer `0x18018A550`<br>`0x2C64E0` LevelAreaInnerLayer::init &middot; detour `0x180189FA0` &middot; installer `0x180154430` |
| `0x1C3` | 451 | `HACK_ALLOW_LOW_VOLUME` | `0x1800B2EA7` | `0x180186100` | `0x37E130` PauseLayer::musicSliderChanged &middot; detour `0x180186290` &middot; installer `0x1800AA770`<br>`0x372A90` PauseLayer::sfxSliderChanged &middot; detour `0x1801862C0` &middot; installer `0x1800AA770`<br>`0x372990` OptionsLayer::musicSliderChanged &middot; detour `0x1801862F0` &middot; installer `0x18015E370`<br>`0x372A90` OptionsLayer::sfxSliderChanged &middot; detour `0x180186370` &middot; installer `0x18015E370` |
| `0x1C6` | 454 | `HACK_THREAD_PRIORITY` | `0x18018ACDC` | — | **callback unresolved** (no `lea rcx,[rip+disp]` within 0x180 bytes) |
| `0x1C9` | 457 | `HACK_MAIN_MENU_PLAY` | `0x1800B308D` | `0x180189260` | `0x331F70` MenuGameLayer::tryJump &middot; detour `0x180189390` &middot; installer `0x180189450`<br>`0x332240` MenuGameLayer::update &middot; detour `0x1801893A0` &middot; installer `0x180189450`<br>`0x333600` MenuGameLayer::ccTouchBegan &middot; detour `0x180189400` &middot; installer `0x180189630` |
| `0x1CB` | 459 | `HACK_PITCH_SHIFTER` | `0x1800B31F7` | `0x18018A840` | symbol-resolved (module + mangled export) |
| `0x1CD` | 461 | `HACK_FAST_CHESTS` | `0x1800B300C` | `0x180187820` | `0x3CDDF0` RewardUnlockLayer::init &middot; detour `0x180187CE0` &middot; installer `0x180187D80`<br>`0x3CEDE0` RewardUnlockLayer::playRewardEffect &middot; detour `0x180187D00` &middot; installer `0x180187FD0`<br>`0x3D0480` RewardUnlockLayer::showCloseButton &middot; detour `0x180187D20` &middot; installer `0x180187FD0`<br>symbol-resolved (module + mangled export)<br>symbol-resolved (module + mangled export) |
| `0x1CF` | 463 | — | `0x1800B2F95` | `0x180186D10` | symbol-resolved (module + mangled export) |
| `0x1D1` | 465 | — | `0x1800B32EA` | `0x18018C040` | `0x27F8D0` GJListLayer::init &middot; detour `0x18018C110` &middot; installer `0x18018C1A0`<br>`0xAB610` CustomListView::loadCell &middot; detour `0x18018C170` &middot; installer `0x18018C2F0` |
| `0x1D5` | 469 | — | `0x1800B3273` | `0x18018AEB0` | symbol-resolved (module + mangled export)<br>symbol-resolved (module + mangled export)<br>symbol-resolved (module + mangled export) |
| `0x1D9` | 473 | — | `0x1800B2F1E` | `0x1801863B0` | `0x3C020` BoomListView::init &middot; detour `0x1801864E0` &middot; installer `0x180186750`<br>`0xB0D70` LevelCell::loadLocalLevelCell &middot; detour `0x180186560` &middot; installer `0x180186B60`<br>`0xAE230` LevelCell::loadCustomLevelCell &middot; detour `0x180186620` &middot; installer `0x180186B60` |
| `0x1DD` | 477 | — | `0x1800B3104` | `0x180189890` | `0x5C530` FMODAudioEngine::fadeOutMusic &middot; detour `0x180189900` &middot; installer `0x180189910` |
| `0x1E3` | 483 | `HACK_SAFE_MODE_POPUP` | `0x1800B3527` | `0x18019A3A0` | `0x2FC430` LevelInfoLayer::onPlay &middot; detour `0x18019A4D0` &middot; installer `0x18019A8E0`<br>`0xD7E50` EditLevelLayer::onPlay &middot; detour `0x18019A520` &middot; installer `0x18019AAC0`<br>`0x31D880` LevelPage::onPlay &middot; detour `0x18019A570` &middot; installer `0x18015CB80` |
| `0x1E8` | 488 | — | `0x1800B3416` | `0x180197020` | `0x23B040` GJBaseGameLayer::resetLevelVariables &middot; detour `0x180197220` &middot; installer `0x180172C30`<br>`0x2377B0` GJBaseGameLayer::getModifiedDelta &middot; detour `0x180197250` &middot; installer `0x180196E30`<br>`0x237850` GJBaseGameLayer::update &middot; detour `0x180197270` &middot; installer `0x180172EC0`<br>`0x23BCF0` GJBaseGameLayer::updateCamera &middot; detour `0x180197440` &middot; installer `0x180172EC0`<br>`0x261DA0` GJEffectManager::updateSpawnTriggers &middot; detour `0x180197490` &middot; installer `0x180197BB0` |
| `0x1ED` | 493 | — | `0x1800B3492` | `0x180198030` | **no hook install** |

## Worked example: Solid Wave Trail read end to end

The `0x18C` (`HACK_SOLID_WAVE_TRAIL`) row is the model for promoting a row from "operands
read" to "behaviour understood":

| Item | Value |
| --- | --- |
| Registration | `0x1800B1A06` (`mov edx,0x18C`) |
| Callback | `FUN_1801728C0` |
| Hook 1 | `PlayerObject::setupStreak`, RVA `0x3887C0`, detour `0x180172A80`, installer `FUN_1800AB230` |
| Hook 2 | `PlayerObject::updateStreakBlend`, RVA `0x388CA0`, detour `0x180172B00`, installer `FUN_180167B20` |
| Disable path | `0x180172926` / `0x18017298F` call the unregister routine `FUN_180108A80` |

This closes the loop with [cosmetic-feature.md](cosmetic-feature.md), which found the
blend-function toggle in `FUN_1801EECA0` (solid `0x30300000001` vs blended `0x100000302`):
the toggle's callback hooks the two `PlayerObject` streak routines that apply that blend
state.

## The one registration that installs no hook

`0x32` (`HACK_EDITOR_EXTENSION`, registration `0x1800B0242`, callback `FUN_18015F4F0`)
does not follow the hook shape: it builds `std::string`s and calls `0x1802048F0` and
`0x180204A60`, and no resolver or installer call appears anywhere in the function. The tag
is therefore registered through the same binder but its callback extends the editor rather
than detouring a function. UNVERIFIED: what those two callees do and which strings are
passed (they are built on the stack, so the values were not read this pass).

## Leads this creates

- **No Death Effect also hooks `destroyPlayer`.** `FUN_180169330` installs *two* hooks, not
  one: `PlayerObject::playDeathEffect` (RVA `0x37EF10`, detour `0x180169400`, a bare `ret`)
  and `PlayLayer::destroyPlayer` (RVA `0x3B39D0`, detour `0x180169410`, installer
  `FUN_180169480`). The second is a candidate explanation for the changelog's "fix No Death
  Effect in duals" wording and was not previously recorded.
- The 46 unlabelled tags are a bounded, closed set. Each already has a
  target and a detour; identifying them is a matter of reading the detour, not searching.
  Four such detours are already read in [render-hooks.md](render-hooks.md#evidence---three-runtime-assigned-cosmetic-tags-2026-10-25):
  three are one-byte `ret` suppressions and one hides a child node through vtable slot
  `+0x140`.
- The tags added by the full-section scan (for example `0x175`, `0x12A`, `0x170`, `0x129`,
  `0x138`, `0x12E`, `0x1C6`, `0x1E8`, `0x1ED`) are the non-cosmetic half of the tag space:
  noclip, frame stepper, checkpoint limits, instant complete, confirmation dialogs, thread
  priority. They are the rows that link this table to the gameplay notes, and
  twenty of them already have static labels.

## Method

1. `scripts/tag_callback_map.py` scans all of `.text` for the four-instruction marker and
   produced tag -> (registration, callback).
2. The static label map was parsed for `label <-> value` pairs and joined on the tag.
3. `scripts/callback_hooks.py` scanned each callback: anchor on `call 0x18010A370`, read the
   preceding `mov edx,imm32`, then the `lea r8`/`lea r9` operands and the following
   installer `call`. The scan is bounded by the next callback in the table, so it cannot
   wander into a neighbour.
4. Four callbacks with hand-derived facts were used as the check: No Shake
   (`0x3A5B00` / `0x180171930` / `FUN_1800AB5B0` / `"PlayLayer::init"`), No Death Effect
   (`0x37EF10` / detour `0x180169400`), No Wave Trail (`0x2B8EF0` /
   `"HardStreak::updateStroke"`) and No Shaders (both ShaderLayer rows, installers
   `FUN_180171710`, detours `0x1801716F0` / `0x180171700`). All four reproduce exactly.

## Open questions

- UNVERIFIED: the names of the 46 runtime-assigned tags. They have no entry
  in the static map, so the name must come from `hackpro.json` (user data) or from the UI
  label that flips them.
- UNVERIFIED: why the static map stops at tag 483 while the runtime-assigned space continues
  past it.
- UNVERIFIED: the callback for tags `0x1BD` (`0x1800A8206`) and `0x1C6`
  (`0x18018ACDC`) — both registrations sit outside the initialiser and no `lea rcx` precedes
  them within the search window.
- UNVERIFIED: which stack argument carries the hook order. The pattern
  `mov dword [rsp+0x20],imm32` is present in some callbacks with `0xffffffe0` (matching the
  `order` column of the untracked `_hook_sites.tsv`) but the same slot also holds other
  values, so it was not tabulated.
