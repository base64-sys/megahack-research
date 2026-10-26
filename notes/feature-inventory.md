# Feature inventory (changelog-derived)

- **Question** - which features does Mega Hack v9.1.3 ship, and which are mapped?
- **Baseline** - Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Source** - `samples/absolllute.hackmega/changelog.md` (release notes).

## Status vocabulary

| Status | Meaning |
| --- | --- |
| `unmapped` | No cluster assignment or code link established. |
| `cluster` | Categorised by changelog wording to the subsystem note that *should* cover it. This is a lead, **not** evidence that the note mentions the feature, names its settings keys, or locates its implementing code. |
| `mapped` | The feature has its own evidence: a note that names it, its settings key(s) and default, and/or its implementing function(s) with VAs. |
| `alias` / `dead` | A restatement of an earlier feature / absent from this build, with the evidence for that call. |

All 389 rows below were `cluster` at the 2026-10-23 baseline. **Eighty-six are now
`mapped`**; the [Mapped rows](#mapped-rows) table below is the authoritative list, with
tag, registration site and hook address for each one.

The 2026-10-24 batch promoted eighteen hook-registered toggles — No (C) Mark, No Death
Effect, No Particles, No Respawn Flash, No Shake, No Portal Lightning, No Pulse, No
Trail, No Wave Trail, No Wave Trail Behind, Wave Pulse Size, No Wave Pulse, Trail
Cutting, No Robot Fire, No Robot/Swing Fire, Classic Particles, No "Do Not" Flip, and
Auto Deafen — each by the same pattern: a `mov edx, <tag>` / `call FUN_18008f910`
registration in the shared initialiser `FUN_1800afae0`, plus a callback that installs
or removes the hook. **Added Hide Player** (2026-10-25) added the detour body as well:
it forces `edx=0` and jumps to the visibility setter. **Added No Orb Ring** and
**Added No Spider Dash** followed from one-byte `ret` detours on
`RingObject::spawnCircle` and `PlayerObject::playSpiderDashEffect`; both tags are
runtime-assigned and have no static label, so those two bindings rest on the target
plus, for Spider Dash, the `COSMETIC/HACK_NO_SPIDER_DASH` settings key. **Four labelled
toggles recovered by the full-`.text` registration scan** (Instant Complete, Checkpoint
Limit Bypass, Allow Low Volume, Fast Chests) needed no inference: their static
`HACK_*` label matches a changelog row verbatim, and their detours were read. **Four more
came from detour bodies** (Place Over, Confirm Practice, Confirm Reset, Confirm
Full-Reset), including a shared confirmation-dialog wrapper. **Show Triggers, Main Menu
Gameplay, Show Layout and the three editor/UI toggles below** (Text Length Bypass, Hide UI,
Smooth Editor Trail) each came from reading every detour installed by the callback. **Hide
Pause Button, Hide Pause Menu and the Respawn Time modifier** were settled the same way: both
hide toggles end in the same vtable slot `+0x140` called with `edx=0`, and the respawn detour
converts a configured millisecond value to seconds before scheduling the action. **Stop
Triggers on Death and Wave Trail Fix** close the last two "target known, body unread" rows:
the first swaps `GJBaseGameLayer::update` for `getModifiedDelta` while a player's `+0x9C0`
byte is set, and the second replaces `HardStreak::updateStroke` outright with a two-ribbon
`CCDrawNode` redraw. **Safe Mode Popup** finishes the last short-detour group: three
code-cave detours on the three `onPlay` entry points show a "Safe Mode" confirmation
when `areCheatsEnabled()` is true and the suppression byte `0x18080CBB0` is clear. **Replay Last
Checkpoint** closes the endscreen path: its single tag reroutes `EndLevelLayer::onReplay` to a
different host procedure when a play-layer byte is set. Auto Song Downloader closed on the same day from a post-hook that defers a
`CCCallFuncO` into host RVA `0xCABC0`. **Auto Pickup Coins and Collect Coins in Practice**
closing a week later were the same mechanism from two sides: both funnel a coin object into
host RVA `0x2161B0`, one from a post-`resetLevel` pass and one from the object scan inside
`collisionCheckObjects`. **Toolbox Button Bypass** replaced two editor steppers outright, so
they count without a host bound. **Jumpscare** builds its own overlay and sound from
`jumpscare.png`/`jumpscare.mp3` after a probability gate passes on death. **Practice Bug Fix**
re-applies the newest checkpoint's two player-state blocks after `resetLevel`, one tag serving
both its changelog wordings. The other 303 rows
remain `cluster`.

A keyword check at the baseline found that **185 of 389** rows point at a note that
never mentions the feature at all, so for those even the cluster assignment is only a
lead. That count predates every promotion since the baseline; the method is recorded in
[m3-validation.md](m3-validation.md).

## Mapped rows

Every registration address and callback in this table is checked against `_original.dll`
by `scripts/check_inventory_evidence.py`; a mismatch fails the check rather than being
reported as progress.

| Feature | Evidence |
| --- | --- |
| Added Safe Mode | tag 482 (`0x1E2`, `HACK_SAFE_MODE`) read by `FUN_18019B5F0`; the gate vetoes `GJGameLevel::savePercentage` (`FUN_180199DF0`) and marks `PlayLayer::levelComplete` (`FUN_180199E30`). See [safe-mode-feature.md](safe-mode-feature.md). |
| Added Auto Safe Mode | tag 481 (`0x1E1`, `HACK_AUTO_SAFE_MODE`) plus `DAT_18080CBD8`, the `hasCheatedInAttempt` cache refreshed by `FUN_18019B330` after `resetLevel` (`FUN_180199E80`). See [safe-mode-feature.md](safe-mode-feature.md). |
| Added Disable Progress to Noclip | tag 73 (`0x49`, `CHECKBOX_NOCLIP_DISABLE_PROGRESS`) enters the same gate when `FUN_180199520() < 1.0`; the progress accumulators are in `FUN_1801992B0`. See [noclip-feature.md](noclip-feature.md). |
| Added Show Hitboxes | tag 192 (`0xC0`, `HACK_SHOW_HITBOXES`) master toggle; gate `FUN_180203EE0`, draw `FUN_180203F60`→`FUN_180204000`/`FUN_180204150`; per-object draw `FUN_180203070`. Tag 0xC1 (193), 0xD9 (217), 0xDA (218) are runtime-assigned Map A. See [show-hitboxes-feature.md](show-hitboxes-feature.md). |
| Selected Objects Only to Show Hitboxes | runtime-assigned tag 193 (`0xC1`); the hitbox renderer is installed unconditionally from `0x1800AFB2B` (`call 0x1801582C0`, which hooks `PlayLayer::updateVisibility` RVA 0x3AF3A0 with detour `0x180158740`); inside `FUN_180204000` the flag at `0x18020409B` chooses between the selection array `[PlayLayer+0x31A0..+0x31A8]` and the all-object path at `0x180204109`. See [show-hitboxes-feature.md](show-hitboxes-feature.md#evidence--selected-objects-only-2026-10-26). |
| Added Show Hitboxes on Death | tag 216 (`0xD8`, `HACK_SHOW_HITBOXES_ON_DEATH`); gate `FUN_180203EE0` branch 2, death-object draw via `FUN_180204000` using `DAT_18080CB80` from the `destroyPlayer` callback. See [show-hitboxes-feature.md](show-hitboxes-feature.md). |
| Added Show Hitbox Trail (Requires Show Hitboxes) | tag 224 (`0xE0`, `HACK_SHOW_HITBOXES_TRAIL`) master toggle; gate `FUN_1801809A0`, trail draw via `FUN_180202BD0` over global point buffer; opacity tag 0xE4 (228). See [show-hitboxes-feature.md](show-hitboxes-feature.md). |
| Added Frame Stepper | tag 302 (`0x12E`, `HACK_FRAME_STEPPER`); registrar `FUN_180173FD0` toggles two UILayer hooks (`onPause` RVA 0x4CDF50, `handleKeypress` RVA 0x4CD830). Tag 357 (0x165, runtime-assigned Map B) is an alternate enable. See [gameplay-hooks.md](gameplay-hooks.md). |
| Added StartPos Switcher | tag 130 (`0x82`, `HACK_STARTPOS_SWITCHER`); gate `FUN_180185710` controls next/prev switching via `FUN_1801857F0`/`FUN_180185910`. Tags 0xA6/0xA7 (166/167) are runtime-assigned Map A. See startpos-switcher-feature.md. |
| Added Solid Wave Trail | tag 396 (`0x18C`, `HACK_SOLID_WAVE_TRAIL`); blend-func toggle in `FUN_1801EECA0` (solid 0x30300000001 vs blended 0x100000302); registered in `FUN_1800afae0` at `0x1800b1a06`, callback `FUN_1801728c0` hooks `PlayerObject::setupStreak` (RVA 0x3887C0) and `PlayerObject::updateStreakBlend` (0x388CA0). See cosmetic-feature.md and [hook-registrations.md](hook-registrations.md#worked-example-promoting-a-candidate-row-to-verified). |
| Added Thread Priority | tag 454 (`0x1C6`, `HACK_THREAD_PRIORITY`); `FUN_18018ADF0`/`FUN_18018AE50` call `SetThreadPriority`. Tag 0x1C7 (455) is the priority level. See render-hooks.md. |
| Added Transition Customiser (with duration) | tag 449 (`0x1C1`, `HACK_NO_TRANSITION`); hooks `CCTransitionFade::create` via `FUN_18018B7F0` (registered by `FUN_18018AEB0`). Tags 0x1D6 (470, duration) and 0x1D8 (472, opacity) are runtime-assigned. See ui-addons.md. |
| Added Pitch Shifter | tag 459 (`0x1CB`, `HACK_PITCH_SHIFTER`); listener `FUN_18018A740` (tag 0x1CC = 460) calls `FUN_18018A800` → `FUN_18020DA00` (applies pitch to FMOD channels 0–4). See audio-sync-feature.md. |
| Added TPS Bypass | tag 490 (0x1EA); hooks `GJBaseGameLayer::getModifiedDelta` (RVA 0x2377B0) via `FUN_180196CA0` (registered by `FUN_180196C30`). Tags 0x1E6 (486, TPS value) and 0x1E7 (487, sub-toggle) are runtime-assigned. See gameplay-hooks.md. |
| Added Borderless Fullscreen | tag 492 (0x1EC, runtime-assigned Map B); `FUN_1801968F0` (vtable 0x180831A2C) calls `FUN_1801964D0(1)` which toggles window border via `SetWindowLongPtrW` + NOP-patch. See render-hooks.md. |
| Added multi-lingual support | language-key-driven: `FUN_180197DA0` registers `FUN_180197EA0` on `GameManager::setGameVariable` (RVA 0x182EB0) toggling hack 0x1ED (493) on key "0025" and setting config 0x1EE (494) on key "0170". Listener `FUN_180197F40` updates UI. See ui-addons.md. |
| Added Interface tab to disable Mega Hack UI changes | tag 496 (0x1F0, runtime-assigned Map B); `FUN_1801985F0` hooks `EndLevelLayer::customSetup` (RVA 0x132E50) and adds "v9.1.3" label unless tag is enabled. See ui-addons.md. |
| Added Menu Snow in December | tag 498 (0x1F2, runtime-assigned Map B); `FUN_180154AD0` checks December via `_time64`, registers snow particle creator `FUN_180154C10` on `MenuLayer::init` and listener `FUN_1801550F0`. See cosmetic-feature.md. |
| Added Noclip Accuracy | tag 497 (0x1F1, runtime-assigned Map B); `FUN_180152010` hooks `EndLevelLayer::showLayer` (RVA 0x134CA0) and draws "Noclip: {:.2f}%" unless tag enabled. Also reads `HACK_NOCLIP` (tag 72). See noclip-feature.md. |
| Added Corrective Music Sync | tag 502 (0x1F6, runtime-assigned Map B); `FUN_18019C420` registers listener for tag 0x1F7 (503) and hooks `FMOD::ChannelControl::setPitch` + `FMODAudioEngine::setMusicTimeMS`. `FUN_18019C6D0` scales pitch by `DAT_1807F9420`. See audio-sync-feature.md. |
| Added NoClip Accuracy screen tint | tag 75 (`CHECKBOX_NOCLIP_TINT`); `FUN_18017A290` hooks `PlayLayer::destroyPlayer` (RVA 0x3B39D0) and applies tint overlay via `DAT_18080C710`. Tags 77/78 (player 1/2) and 0x4C (colour) are runtime-assigned. See noclip-feature.md. |
| Added Custom Background (successor to Transparent BG) | tag 463 (0x1CF, runtime-assigned Map B); `FUN_180186D10` registers listener for tag 0x1D0 (464) and hooks `CCTransitionScene::initWithDuration`. `FUN_180186FC0` calls `FUN_180187030` to hide background when enabled. See cosmetic-feature.md. |
| Added Hitbox Trail | tag 360 (0x168, runtime-assigned Map B); `FUN_1801589F0` creates draw node 0x1E6813C with visibility controlled by tag. Main trail draw is `FUN_1801809A0` (tag 224). See show-hitboxes-feature.md. |
| Added Accurate Percentage | tag 429 (0x1AD, runtime-assigned Map B); `FUN_180165110` hooks `PlayLayer::updateProgressbar` (RVA 0x3B23C0) and formats percentage with configurable decimals (tag 0x1AE = 430). See best-run-feature.md. |
| Added Noclip Limits | tag 83 (`HACK_NOCLIP_LIMITS`); `FUN_18017A290` implements accuracy limit (tag 0x55/0x54) and death limit (tag 0x57/0x56) gating tint overlay. See noclip-feature.md. |
| Added 0% Practice Complete | tag 296 (`HACK_0_PRACTICE_COMPLETE`, static map entry #0); read via `hackpro::Config` vtable (`FUN_1800917B0` at 0x180544FF0). No gameplay code reads it directly. See gameplay-hooks.md. |
| Added No Particles | tag 402 (`HACK_NO_PARTICLES`, 0x192); registered in `FUN_1800afae0` at `0x1800b11a8`, callback `FUN_1801703b0` (toggles cocos2d hook based on `*param_2`). See no-particles-feature.md. |
| Added No Shake | tag 409 (`HACK_NO_SHAKE`, 0x199); registered in `FUN_1800afae0` at `0x1800b1560`, callback `FUN_1801718C0` (hooks `PlayLayer::init`, RVA 0x3A5B00, detour 0x180171930). See no-shake-feature.md. |
| Added No Portal Lightning | tag 416 (`HACK_NO_PORTAL_LIGHTNING`, 0x1A0); registered in `FUN_1800afae0` at `0x1800b1296`, callback `FUN_180171170` (hooks `GJBaseGameLayer::lightningFlash`, RVA 0x246D00). See no-portal-lightning-feature.md. |
| Added No Pulse | tag 413 (`HACK_NO_PULSE`, 0x19D); registered in `FUN_1800afae0` at `0x1800b1384`, callback `FUN_1801713e0` (hooks `PlayLayer::updateVisibility`, RVA 0x3AF3A0); detour `0x180171450` writes 0.5f to `[[layer+0x3270]+0x23C]`, `[[layer+0x3270]+0x244]` and `[0x180128CF0()+0x210]`, then tail-jumps the cached original. See [no-pulse-feature.md](no-pulse-feature.md#evidence--no-pulse-detour-body-2026-10-26). |
| Added No Trail | tag 395 (`HACK_NO_TRAIL`, 0x18B); registered in `FUN_1800afae0` at `0x1800b16c5`, callback `FUN_180171db0` (hooks `cocos2d::CCMotionStreak::draw`). See no-trail-feature.md. |
| Added No Wave Trail | tag 397 (`HACK_NO_WAVE_TRAIL`, 0x18D); registered in `FUN_1800afae0` at `0x1800b17b3`, callback `FUN_1801721f0` (hooks `HardStreak::updateStroke`, RVA 0x2B8EF0). See no-wave-trail-feature.md. |
| Added No Wave Trail Behind | tag 414 (`HACK_NO_WAVE_TRAIL_BEHIND`, 0x19E); registered in `FUN_1800afae0` at `0x1800b182a`, callback `FUN_180172280` (hooks `PlayerObject::activateStreak`, RVA 0x3A0090). See no-wave-trail-behind-feature.md. |
| Added Wave Pulse Size | tag 407 (`HACK_WAVE_PULSE_SIZE`, 0x197); registered in `FUN_1800afae0` at `0x1800b1c59`, callback `FUN_180173920` (hooks `PlayerObject::update`, RVA 0x388D80). See wave-pulse-size-feature.md. |
| Added Trail Cutting | tag 392 (`HACK_TRAIL_CUTTING`, 0x188); registered in `FUN_1800afae0` at `0x1800b1be2`, callback `FUN_1801732c0` (hooks `cocos2d::CCMotionStreak::initWithFade`). See trail-cutting-feature.md. |
| Added No Robot Fire | tag 440 (0x1B8, runtime-assigned); registered in `FUN_1800afae0` at `0x1800b1472`, callback `FUN_180171550` (hooks `PlayerObject::toggleRobotMode`). See no-robot-swing-fire-feature.md. |
| Added No Robot/Swing Fire | tags 440/441 (0x1B8/0x1B9, runtime-assigned); registered in `FUN_1800afae0` at `0x1800b1472`/`0x1800b164e`, callbacks `FUN_180171550`/`FUN_180171ca0` (hooks `PlayerObject::toggleRobotMode`/`toggleSwingMode`). See no-robot-swing-fire-feature.md. |
| Added Classic Particles | tag 390 (`HACK_CLASSIC_PARTICLES`, 0x186); registered in `FUN_1800afae0` at `0x1800b085c`, callback `FUN_180166270` (hooks `cocos2d::CCParticleSystem::loadScaledDefaults`). See classic-particles-feature.md. |
| Added No "Do Not" Flip | tag 412 (`HACK_NO_DO_NOT_FLIP`, 0x19C); registered in `FUN_1800afae0` at `0x1800b0e67`, callback `FUN_1801696e0` (hooks `EndLevelLayer::customSetup` + `PlayLayer::setupHasCompleted`). See no-do-not-flip-feature.md. |
| Added Auto Deafen | tag 279 (`HACK_AUTO_DEAFEN`, 0x117); registered in `FUN_1800afae0` at `0x1800b1dc8`, callback `FUN_180174a80` (hooks `PlayLayer::resetLevel`, RVA 0x3B8EB0). See auto-deafen-feature.md. |
| Added Hide Player | tag 386 (`0x182`, `HACK_HIDE_PLAYER`); registered in `FUN_1800afae0` at `0x1800b0aaf`, callback `FUN_180167a60` hooks `PlayerObject::toggleVisibility` (RVA 0x38B440), detour `0x180167b10` forces `edx=0` before `jmp 0x1801a9b70`. See [render-hooks.md](render-hooks.md#evidence---hide-player-verified-2026-10-25). |
| Added No Orb Ring | tag 420 (`0x1A4`, runtime-assigned — no static `HACK_*` label); registered in `FUN_1800afae0` at `0x1800b1131`, callback `FUN_18016a370` hooks `RingObject::spawnCircle` (RVA 0x4A1DB0), detour `0x18016a3e0` is a bare `ret`. See [render-hooks.md](render-hooks.md#evidence---three-runtime-assigned-cosmetic-tags-2026-10-25). |
| Added No Spider Dash | tag 434 (`0x1B2`, runtime-assigned); registered in `FUN_1800afae0` at `0x1800b15d7`, callback `FUN_1801719c0` hooks `PlayerObject::playSpiderDashEffect` (RVA 0x395170), detour `0x180171a30` is a bare `ret`. Key `COSMETIC/HACK_NO_SPIDER_DASH`. See [render-hooks.md](render-hooks.md#evidence---three-runtime-assigned-cosmetic-tags-2026-10-25). |
| Added Instant Complete | tag 89 (`0x59`, `HACK_INSTANT_COMPLETE`, static map `0x1800633d2`); registered at `0x1800b25b9`, callback `FUN_1801789e0` hooks `PlayLayer::postUpdate`/`levelComplete`/`updateTimeWarp`; flag `0x18080C8C8` scopes the completion. See [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25). |
| Added Checkpoint Limit Bypass | tag 298 (`0x12A`, `HACK_CHECKPOINT_LIMIT_BYPASS`); registered at `0x1800b2092`, callback `FUN_180176250` hooks `PlayLayer::storeCheckpoint` (RVA 0x3B74A0), detour `0x1801762c0`. See [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25). |
| Added Allow Low Volume | tag 451 (`0x1C3`, `HACK_ALLOW_LOW_VOLUME`); registered at `0x1800b2ea7`, callback `FUN_180186100` hooks the four music/sfx slider callbacks; detours recompute the float before the setter `0x180129980`. See [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25). |
| Added Fast Chests | tag 461 (`0x1CD`, `HACK_FAST_CHESTS`); registered at `0x1800b300c`, callback `FUN_180187820` hooks `RewardUnlockLayer::init`/`playRewardEffect`/`showCloseButton`, detours scope flag `0x18080C8E0`. See [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25). |
| Added Place Over | tag 51 (`0x33`, `HACK_PLACE_OVER`, static map `0x180062FAA`); registered at `0x1800b0511`, callback `FUN_180163000` hooks `LevelEditorLayer::typeExistsAtPosition` (RVA 0x2D4A50), detour `0x180163070` returns 0 unconditionally. See [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25). |
| Added Confirm Practice | tag 311 (`0x137`, `HACK_CONFIRM_PRACTICE`); registered at `0x1800b2273`, callback `FUN_1801772e0` hooks `PauseLayer::onPracticeMode` (RVA 0x37E1C0), detour `0x180177350`. See [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25). |
| Added Confirm Reset | tag 313 (`0x139`, `HACK_CONFIRM_RESET`); registered at `0x1800b22ea`, callback `FUN_1801774b0` hooks `PauseLayer::onRestart` (RVA 0x37E410), detour `0x180177520`. See [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25). |
| Added Confirm Full-Reset | tag 314 (`0x13A`, `HACK_CONFIRM_FULL_RESET`); registered at `0x1800b2361`, callback `FUN_180177680` hooks `PauseLayer::onRestartFull` (RVA 0x37E4B0), detour `0x1801776f0`. See [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25). |
| Added Show Triggers | tag 286 (`0x11E`, `HACK_SHOW_TRIGGERS`, static map `0x1800640F7`); registered at `0x1800b2cc6`, callback `FUN_180183790` installs three hooks (`addObject`, `addToSection`, `customSetup`) sharing flag `0x18080C840`; the customSetup detour accepts IDs 0xB53..0xBD9. See [render-hooks.md](render-hooks.md#evidence--show-triggers-2026-10-25). |
| Added Text Length Bypass | tag 37 (`0x25`, `HACK_TEXT_LENGTH`, static map `0x180064420`); registered at `0x1800afe13`, callback `FUN_18015d6d0` hooks `CCTextInputNode::onTextFieldAttachWithIME` (RVA 0x508F0); detour `0x18015d740` sets the cap to `0x7FFFFFFF` via `0x1801e6d20` before tail-jumping the original. See [ui-addons.md](ui-addons.md#evidence--text-length-bypass-2026-10-25). |
| Added Hide UI | tag 53 (`0x35`, `HACK_HIDE_UI`, static map `0x180063372`); registered at `0x1800b0330`, callback `FUN_18015fa70` hooks `EditorUI::init` (RVA 0xDF390); detour `0x18015fb20` calls vtable `+0x140` with `edx=0`. See [render-hooks.md](render-hooks.md#evidence--hide-ui-2026-10-25). |
| Added Smooth Editor Trail | tag 55 (`0x37`, `HACK_SMOOTH_EDITOR_TRAIL`, static map `0x180064222`); registered at `0x1800b058d`, callback `FUN_180163ec0` hooks `LevelEditorLayer::postUpdate` (RVA 0x2E0F40); detour `0x180163f30` stores `0x3D088889` (1/30 s) at `[layer+0x3898]`. See [creator-feature.md](creator-feature.md#evidence--smooth-editor-trail-2026-10-25). |
| Added Show Layout | tag 260 (`0x104`, `HACK_SHOW_LAYOUT`, static map `0x180064095`); registered at `0x1800b2b5c`, callback `FUN_18017f0f0` installs six hooks (`resetLevel`, `applyEnterEffect`, `applyCustomEnterEffect`, `triggerObject`, `addGlow`, `saveActiveColors`); the resetLevel detour reads Map A tags 262/263. See [gameplay-hooks.md](gameplay-hooks.md#evidence--show-layout-2026-10-25). |
| Added Main Menu Gameplay | tag 457 (`0x1C9`, `HACK_MAIN_MENU_PLAY`, static map `0x180063529`); registered at `0x1800b308d`, callback `FUN_180189260` hooks `MenuGameLayer::tryJump`/`update`/`ccTouchBegan` (RVAs 0x331F70, 0x332240, 0x333600) around flag `0x18080C920`. See [gameplay-hooks.md](gameplay-hooks.md#evidence--main-menu-gameplay-2026-10-25). |
| Added No (C) Mark | tag 44 (`HACK_NO_C_MARK`, 0x02C); registered in `FUN_1800afae0` at `0x1800b049a`, callback `FUN_180162d20` (hooks `ShareLevelLayer::onShare`, RVA 0x490270). See no-c-mark-feature.md. |
| Added No Respawn Flash | tag 398 (`HACK_NO_RESPAWN_FLASH`, 0x18E); registered in `FUN_1800afae0` at `0x1800b13fb`, callback `FUN_1801714c0` (hooks `PlayerObject::playSpawnEffect`, RVA 0x397480). See no-respawn-flash-feature.md. |
| Added No Death Effect mod | tag 399 (`HACK_NO_DEATH_EFFECT`, 0x18F); registered in `FUN_1800afae0` at `0x1800b0df0`, callback `FUN_180169330` (hooks `PlayerObject::playDeathEffect`, RVA 0x37EF10, with a bare-`ret` detour at `0x180169400`). See no-death-effect-feature.md. |
| Added Hide Pause Button | tag 385 (`0x181`, `HACK_HIDE_PAUSE_BUTTON`, static map `0x18006326C`); registered at `0x1800b0b26`, callback `FUN_180167d10` hooks `UILayer::init` (RVA 0x4CCB00); detour `0x180167d80` hides the child at `UILayer+0x238` via vtable `+0x140` with `edx=0`. See [render-hooks.md](render-hooks.md#evidence--hide-pause-button-and-hide-pause-menu-2026-10-25). |
| Added Hide Pause Menu | tag 410 (`0x19A`, `HACK_HIDE_PAUSE_MENU`, static map `0x1800632CF`); registered at `0x1800b0b9d`, callback `FUN_180167fa0` hooks `PauseLayer::customSetup` (RVA 0x37C870); detour `0x1801680a0` runs `0x1801a2c30` then hides the layer via vtable `+0x140`. See [render-hooks.md](render-hooks.md#evidence--hide-pause-button-and-hide-pause-menu-2026-10-25). |
| Added Respawn Time modifier | tag 245 (`0x0F5`, `HACK_RESPAWN_TIME`, static map `0x180063E55`); registered at `0x1800b2ae5`, callback `FUN_18017ef50` hooks `PlayLayer::destroyPlayer` (RVA 0x3B39D0); detour converts tag 246 ms to seconds (÷1000.0) and calls `0x18017f000`, which clamps to 2^-23 and applies the tag-501 multiplier when tags 502 and 247 are on. See [gameplay-hooks.md](gameplay-hooks.md#evidence--respawn-time-modifier-2026-10-25). |
| Added Stop Triggers on Death | tag 415 (`0x19F`, `HACK_STOP_TRIGGERS_ON_DEATH`, static map `0x1800643C4`); registered at `0x1800b1af4`, callback `FUN_180172de0` hooks `GJBaseGameLayer::update` (RVA 0x237850); detour `0x180172e50` forwards to the cached original unless `[host_0x17B4A0+0x208] == this` and a player slot (`layer+0xDA0`/`0xDA8`) has byte `+0x9C0` set, in which case it tail-jumps to the `getModifiedDelta` gateway (RVA 0x2377B0) instead. See [gameplay-hooks.md](gameplay-hooks.md#evidence--stop-triggers-on-death-2026-10-25). |
| Added Wave Trail Fix | tag 411 (`0x19B`, `HACK_WAVE_TRAIL_FIX`, static map `0x180064855`); registered at `0x1800b094a`, callback `FUN_180166a80` hooks `HardStreak::updateStroke` (RVA 0x2B8EF0); detour `0x180166af0` replaces it: `CCDrawNode::clear()`, then two `drawPolygon` ribbons built by `0x180166c40` from the point array at `+0x1A0` (6.0x width in the node colour, 2.0x in white at 0.65x opacity). See [render-hooks.md](render-hooks.md#evidence--wave-trail-fix-2026-10-25). |
| Added Auto Pickup Coins | tag 320 (`0x140`, `HACK_AUTO_PICKUP_COINS`); registered at `0x1800b1f2d`, callback `FUN_1801754c0` hooks `PlayLayer::resetLevel` (RVA 0x3B8EB0); detour `0x180175530` runs the original first, then collects each candidate whose tag-321 flag is set through host RVA `0x2161b0`. See [gameplay-hooks.md](gameplay-hooks.md#evidence--the-two-coin-collection-hooks-2026-10-25). |
| Added Collect Coins in Practice | tag 297 (`0x129`, `HACK_COLLECT_COINS_IN_PRACTICE`); registered at `0x1800b2185`, callback `FUN_180176ea0` hooks `GJBaseGameLayer::collisionCheckObjects` (RVA 0x214960); detour `0x180176f10` collects type ids `0x1F`/`0x16` objects through host RVAs `0x2161b0` and `0x216090` before tail-jumping to the original. See [gameplay-hooks.md](gameplay-hooks.md#evidence--the-two-coin-collection-hooks-2026-10-25). |
| Added Safe Mode Popup | tag 483 (`0x1E3`, `HACK_SAFE_MODE_POPUP`); three code-cave detours (`0x18019a4d0` `LevelInfoLayer::onPlay`, `0x18019a520` `EditLevelLayer::onPlay`, `0x18019a570` `LevelPage::onPlay`) tail-jump to the cached original when byte `0x18080cbb0` is non-zero or `areCheatsEnabled` (`0x18019b350`) is false; otherwise they jump to `0x18019a5c0`, the alert that renders "Safe Mode" / "You have cheats enabled, are you sure you want to play this level?" with Play and Cancel. See [safe-mode-feature.md](safe-mode-feature.md#evidence--the-play-button-popup-2026-10-25). |
| Added Replay Last Checkpoint (all three wordings) | tag 316 (`0x13C`, `HACK_REPLAY_LAST_CHECKPOINT`, static map `0x180063DF7`); registered at `0x1800b2a6e`, callback `FUN_18017ecb0` hooks `EndLevelLayer::onReplay` (RVA 0x134E80); detour `0x18017ed20` reroutes to host RVA 0x1354C0 with `edx=0` when the second argument is null and `[playlayer+0x31f0] == 1`, otherwise to the cached original. One tag and one hook serve the v6.2.0, v7.0 and v8.0.0-alpha9 rows, so they are promoted together. See [gameplay-hooks.md](gameplay-hooks.md#evidence--replay-last-checkpoint-2026-10-25). |
| Added Toolbox Button Bypass | tag 47 (`0x02F`, `HACK_TOOLBOX_BUTTON_BYPASS`, static map `0x1800644EA`); registered at `0x1800b0604`, callback `FUN_180163f50` installs two code-cave replacements: `EditorOptionsLayer::onButtonRows` (RVA 0x291A00) -> `0x180164020` and `onButtonsPerRow` (RVA 0x291970) -> `0x1801641b0`. Both read a direction from `[[arg2]+0x38]`, `inc` the stored count at `+0x3F4`/`+0x3F0` with no ceiling, format it with `"{}"` through `0x18000AE30` and push it into the label at `[[this+0x400]+0x168]`/`[[this+0x3F8]+0x168]`. See [ui-addons.md](ui-addons.md#evidence--toolbox-button-bypass-2026-10-25). |
| Added Practice Bug Fix (both wordings) | tag 317 (`0x13D`, `HACK_PRACTICE_BUG_FIX`, static map `0x180063CD1`, key `LEVEL/HACK_PRACTICE_BUG_FIX`); registered at `0x1800b288d`, callback `FUN_18017b620` installs detour `0x18017b690` on `PlayLayer::resetLevel` (RVA 0x3B8EB0). The detour runs the original first, takes the last `CCArray` element at `[layer+0x3888]`, verifies via RTTI that it has a `cocos2d::CCNode` base, then copies the state blocks at element+0x148 and (when byte `+0xC38 == 1`) element+0x6C0 into `[layer+0xDA0]`/`[layer+0xDA8]` through `0x180207470`. The v8.0.0-alpha10b and v8.0.1-alpha.5 rows are the same toggle. See [gameplay-hooks.md](gameplay-hooks.md#evidence--practice-bug-fix-2026-10-25). |
| Added Jumpscare | tag 291 (`0x123`, `HACK_JUMPSCARE`, static map `0x180063420`); registered at `0x1800b26a7`, callback `0x180178d10` hooks `PlayLayer::destroyPlayer` (RVA 0x3B39D0) with code-cave detour `0x180178d80`, which runs the original through the trampoline `0x1801a3d20`, tests tags 0x127/0x126/0x125/0x124 (only-from-0, time threshold, percentage, probability) in gate `0x180179ae0`, then adds a black `CCLayerColor` carrying `jumpscare.png` to `[layer+0x3540]` at z-order 100 and tail-calls `0x180179260` to play `jumpscare.mp3` and run the fade/scale chain. See [gameplay-hooks.md](gameplay-hooks.md#evidence--jumpscare-2026-10-25). |
| Added Smart StartPos (both rows) | tag 235 (`0x0EB`, `HACK_SMART_STARTPOS`, static map `0x1800641B8`); registered at `0x1800b2db4`, callback `FUN_180183e30` installs detour `0x180183ea0` on `PlayLayer::resetLevel` (RVA 0x3B8EB0), which runs the original and then continues into `0x180183ec0`. That body returns unless `[layer+0x3188] != 0` and the `CCArray` at `[layer+0x3888]` is empty, reads four lists from the layer's `absolllute.megahack` user dictionary through `0x18015afd0`, picks in each the nearest object at or before player 1 by `0x1802044b0`, and applies it under tags 237 (gamemode, `0x1801842e0`), 238 (speed, host RVA `0x3A0D50`), 239 (mini, host RVA `0x3A0700`) and 240 (mirror, host RVA `0x2467D0`). The v8.0.0-alpha5 and v9.0.1 rows share this one hook. See [gameplay-hooks.md](gameplay-hooks.md#evidence--smart-startpos-2026-10-25). |
| StartPos Switcher interface family (six rows) | the interface is installed unconditionally by `0x180184490` from the shared initialiser at `0x1800B2DEE`; its three hooks (`PlayLayer::init` RVA 0x3A5B00 -> `0x1801846C0`, `setupHasCompleted` 0x3A66D0 -> `0x1801846D0`, `UILayer::init` 0x4CCB00 -> `0x180184710`) and the tag-169/170 listeners `0x1801848A0`/`0x1801848E0` are read, and the remaining behaviour lives in tag 130's detours: `0x180185070` reads keybinds 131/132 and quick-switch tag 149 with the 17-entry table `0x1805793A8`, `0x180185CE0` defers through tag 168, and the reset hook `0x180184A80` applies the pending startpos. See [startpos-switcher-feature.md](startpos-switcher-feature.md#evidence--startpos-switcher-family-2026-10-26). |
| Added Auto Music Sync | tag 326 (`0x146`, `HACK_AUTO_MUSIC_SYNC`, store `0x180062AFF`); registered at `0x1800b1eb6`, callback `0x1801751e0` installs `0x180175250` on `PlayLayer::postUpdate` (RVA 0x3B4BA0). The detour skips platformer levels, the `0x18015A2F0`/`0x180129AB0` state checks and non-empty dictionary groups 6/7/8, then calls `0x180175310`, which reads the music channel through `[engine+0x228]`, `getChannel` and `getPosition`, computes the target from the player position (tag 328 selects the source) plus `[engine+0x264]` and `[layer+0xDB0]+0x158`, and re-seats the channel with `setPosition` when the drift exceeds tag 327. See [audio-sync-feature.md](audio-sync-feature.md#evidence--auto-music-sync-2026-10-26). |
| Added Auto Song Downloader | tag 323 (`0x143`, `HACK_AUTO_SONG_DOWNLOAD`, static map `0x180062C24`); registered at `0x1800b1fa4`, callback `FUN_180175640` hooks `LevelInfoLayer::init` (RVA 0x2F7340) with code-cave detour `0x1801756b0`; the detour calls the original first, then runs `CCCallFuncO::create(subObject, <host 0xCABC0>, subObject)` on `[[layer+0x2D0]+0x180]` when that object's vtable slot `+0x148` returns true. See [ui-addons.md](ui-addons.md#evidence--auto-song-downloader-2026-10-25). |

## Toggles outside the 389

The row universe above is the `Added …` changelog bullets. Ten toggles are named elsewhere in
the changelog and are mapped in their own notes, but have no row here: three under
`New For 2.2 Mods`, No Transition only in a fix bullet, five unlock/bypass toggles only in
fix bullets, and Copy Hack only in the settings tree.

| Feature | Evidence |
| --- | --- |
| No Shaders | tag 389 (`HACK_NO_SHADERS`, 0x185); registered in `FUN_1800afae0` at `0x1800b14e9`, callback `FUN_180171620` (hooks `ShaderLayer::performCalculations`, RVA 0x4880B0, and `ShaderLayer::visit`, 0x488A60). See [no-shaders-feature.md](no-shaders-feature.md). |
| No Camera | tag 387 (`HACK_NO_CAMERA`, 0x183); registered in `FUN_1800afae0` at `0x1800b0c14`, callback `FUN_1801680e0` (hooks `CameraTriggerGameObject::triggerObject`, RVA 0x4BD820). See [no-camera-feature.md](no-camera-feature.md). |
| No Camera Zoom | tag 388 (`HACK_NO_CAMERA_ZOOM`, 0x184); registered in `FUN_1800afae0` at `0x1800b0c8b`, callback `FUN_1801682c0` (hooks `EffectGameObject::triggerObject`, RVA 0x4A5F30). See [no-camera-zoom-feature.md](no-camera-zoom-feature.md). |
| No Transition | tag 449 (`HACK_NO_TRANSITION`, 0x1C1); registered at `0x1800b317b`, callback `FUN_180189a70` forces the duration argument of `cocos2d::CCTransitionFade::create` and `cocos2d::CCTransitionMoveInT::create` to 0, and adds three menu-transition hooks. Named only in a changelog fix bullet ("No Transition now works on shop, tower & garage layers"), never in an `Added …` line. See [ui-addons.md](ui-addons.md#evidence--no-transition-2026-10-25). |
| Unlock Icons | tag 32 (`HACK_UNLOCK_ICONS`, 0x020); registered at `0x1800aff01`, callback `FUN_18015dc00` replaces `GameManager::isIconUnlocked` (RVA 0x17C500) and `isColorUnlocked` (0x17C8E0) with `mov al,1; ret`, so colours are covered too. Named only in fix bullets. See [unlock-bypasses.md](unlock-bypasses.md). |
| Music Unlocker | tag 36 (`HACK_MUSIC_UNLOCKER`, 0x024); registered at `0x1800afd25`, callback `FUN_18015d130` hooks `GameStatsManager::isItemUnlocked` (RVA `0x1E5820`) and answers `true` only for argument pair `(12,16)`, forwarding the rest. Named only in fix bullets. See [unlock-bypasses.md](unlock-bypasses.md). |
| Unlock Vaults | tag 33 (`HACK_UNLOCK_VAULTS`, 0x021); registered at `0x1800affef`, callback `FUN_18015e220` **replaces** `CreatorLayer::onSecretVault` (RVA 0x9E590) and `OptionsLayer::onSecretVault` (0x372CD0), building the scene and running the 0.5 s fade itself. Named only in fix bullets. See [unlock-bypasses.md](unlock-bypasses.md#vault-shop-and-treasure-room-the-handlers-are-replaced). |
| Unlock Shops | tag 34 (`HACK_UNLOCK_SHOPS`, 0x022); registered at `0x1800aff78`, callback `FUN_18015dfa0` replaces `SecretRewardsLayer::onShop` (RVA 0x3D66D0) with a forward to `0x1801AD0F0` carrying this plus the button's virtual index. Named only in fix bullets. See [unlock-bypasses.md](unlock-bypasses.md#vault-shop-and-treasure-room-the-handlers-are-replaced). |
| Treasure Room | tag 35 (`HACK_TREASURE_ROOM`, 0x023); registered at `0x1800afe8a`, callback `FUN_18015d960` replaces `CreatorLayer::onTreasureRoom` (RVA 0x9EDA0), building the scene and running the 0.5 s fade itself. Named only in fix bullets. See [unlock-bypasses.md](unlock-bypasses.md#vault-shop-and-treasure-room-the-handlers-are-replaced). |
| Copy Hack | tag 43 (`0x02B`, `HACK_COPY_HACK`, static map `0x180062F43`); registered at `0x1800b00dd`, callback `0x18015e920` installs detour `0x18015e9f0` on `LevelInfoLayer::init` (RVA 0x2F7340) and `0x18015ea50` on `LevelInfoLayer::levelDownloadFinished` (RVA 0x2FBA60); both force the dword at `[level+0x3A8]` to `[level+0x3AC]+1` for the duration of the call and restore it after. No `Added ...` bullet names it and the spoofed fields are unidentified. See [creator-feature.md](creator-feature.md#lead--copy-hacks-two-levelinfolayer-spoofs-2026-10-26). |

Adding them to the counts would mix two different universes. If the M3 criterion is later read
as "every feature named anywhere in the changelog", these nine are the first rows to add — and
No Transition and the unlock bypasses show the gap is wider than `New For 2.2 Mods`: a feature can be named only in a
fix bullet, so a row universe built from `Added …` alone will always miss some.

| Version | Feature (changelog wording) | Status | Notes |
| --- | --- | --- | --- |
| v9.1.0 | Added Icon Stealer | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.1.0 | Added Selected Objects Only to Show Hitboxes | mapped | [show-hitboxes-feature.md](show-hitboxes-feature.md#evidence--selected-objects-only-2026-10-26) | runtime-assigned tag 193 (`0xC1`) read at `0x18020409b` in `FUN_180204000`; when set the pass iterates the selection array at `[PlayLayer+0x31a0]..[+0x31a8]` through `0x180203070`, otherwise falls through to the all-object path. The hitbox renderer itself is installed unconditionally at `0x1800afb2b`. |
| v9.1.0 | Added StartPos Switcher Quick Switch toggle | mapped | [startpos-switcher-feature.md](startpos-switcher-feature.md#evidence--startpos-switcher-family-2026-10-26) | tag 149 (`0x95`) gates the 17-entry shortcut loop in `0x180185070`; the same dispatcher reads keybind tags 131/132 and calls `0x180185CE0` with `list[index - 1]`. |
| v9.1.0-beta.6 | Added StartPos Switcher quick switch | mapped | [startpos-switcher-feature.md](startpos-switcher-feature.md#evidence--startpos-switcher-family-2026-10-26) | Same mechanism as the v9.1.0 toggle: tag 149 plus the shortcut table `0x1805793A8` (tags 150-165, `0x96` duplicated at index 10), reached from the `UILayer::handleKeypress` and `PauseLayer::keyDown` detours. |
| v9.1.0-beta.6 | Added Replay Open Folder | cluster | [replay-feature.md](replay-feature.md) |
| v9.1.0-beta.6 | Added Crash Game to Utilities | cluster | [ui-addons.md](ui-addons.md) |
| v9.1.0-beta.5 | Added Accurate Percentage Better Calculation (fixes StartPos!) | cluster | [best-run-feature.md](best-run-feature.md) |
| v9.1.0-beta.3 | Added CBS Tick Highlighting in Show Hitbox Trail | cluster | [render-hooks.md](render-hooks.md) |
| v9.1.0-beta.3 | Added Show Hitbox Trail Opacity | cluster | [render-hooks.md](render-hooks.md) |
| v9.1.0-beta.3 | Added Trail Always On | cluster | [render-hooks.md](render-hooks.md) |
| v9.1.0-beta.1 | Added Auto-Update | cluster | [ui-addons.md](ui-addons.md) |
| v9.1.0-beta.1 | Added Challenge Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v9.1.0-beta.1 | Added Click Between/On Steps shortcut | cluster | [input-hooks.md](input-hooks.md) |
| v9.1.0-beta.1 | Added Icon Randomiser to the Cosmetic menu | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.1.0-beta.1 | Added Keymaster Vault Room Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v9.1.0-beta.1 | Added RobsVault shortcuts | cluster | [ui-addons.md](ui-addons.md) |
| v9.1.0-beta.1 | Added Show Hitboxes Hide Activated Objects option | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.1.0-beta.1 | Added Show Hitboxes Player Rotation option | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.1.0-beta.1 | Added Velocity & Death options to Status labels | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.1.0-beta.1 | Added show/hide animations to dropdowns and tabs | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.11 | Added "Requires Show Hitboxes" option to Show Hitbox Trail | mapped | [show-hitboxes-feature.md](show-hitboxes-feature.md) | tag 0xE3 (227, runtime-assigned Map A); gates trail draw on `FUN_180203EE0`. See show-hitboxes-feature.md. |
| v9.0.10 | Added Icon Randomiser | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.10 | Added positions top-centre and bottom-centre Status label positions | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.10 | Added Vanilla Progress Bar to Iconic | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.9 | Added Geode DevTools shortcut | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.9 | Added Player 1 on Top | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.8 | Added Auto Clicker | cluster | [input-hooks.md](input-hooks.md) |
| v9.0.8 | Added Auto-Select toggle for Search | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.8 | Added Coin Shower | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.8 | Added No End Shake | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.8 | Added No Robot/Swing Fire | mapped | [render-hooks.md](render-hooks.md) | tags 440/441 (0x1B8/0x1B9, runtime-assigned); registered in `FUN_1800afae0` at `0x1800b1472`/`0x1800b164e`, callbacks `FUN_180171550`/`FUN_180171ca0` (hooks `PlayerObject::toggleRobotMode`/`toggleSwingMode`). See no-robot-swing-fire-feature.md. |
| v9.0.8 | Added Replay option to block inputs | cluster | [replay-feature.md](replay-feature.md) |
| v9.0.8 | Added Show Layout colour options | cluster | [gameplay-hooks.md](gameplay-hooks.md#evidence--show-layout-2026-10-25) — the Show Layout `resetLevel` detour reads Map A tags 262/263, a candidate binding for these options; keys and defaults UNVERIFIED. |
| v9.0.8 | Added Show Trajectory iterations/thickness options | cluster | [show-trajectory-feature.md](show-trajectory-feature.md) |
| v9.0.8 | Added a small delay when auto-selecting Search | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.8 | Added Festive theme | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.7 | Added No Circle Effect | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.4 | Added Best Run options "Only from Current %" and "Show Prefix" | cluster | [best-run-feature.md](best-run-feature.md) |
| v9.0.4 | Added Multiple Editor Trails | cluster | [creator-feature.md](creator-feature.md) |
| v9.0.4 | Added No Dash Fire | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.4 | Added No New Best Popup | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.4 | Added No Spider Dash | mapped | [render-hooks.md](render-hooks.md#evidence---three-runtime-assigned-cosmetic-tags-2026-10-25) | tag 434 (`0x1B2`, runtime-assigned, no static label); registered at `0x1800b15d7`, callback `FUN_1801719c0` hooks `PlayerObject::playSpiderDashEffect` (RVA 0x395170), detour `0x180171a30` is a bare `ret`. Key `COSMETIC/HACK_NO_SPIDER_DASH` in [settings-contents.md](settings-contents.md). |
| v9.0.4 | Added Same Dual Colour to Iconic | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.4 | Added Session Time | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.4 | Added Show Hitboxes Player Collison Blocks | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.0.4 | Added info button to mobile interface dropdowns | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.2 | Added Cheat Safety tab (Safe Mode moved here) | cluster | [safe-mode-feature.md](safe-mode-feature.md) |
| v9.0.2 | Added Disable All Cheats button | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.2 | Added Frozen Animations | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.2 | Added No Ghost Trail | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.1 | Added Accurate Percentage toggle in Best Run | cluster | [best-run-feature.md](best-run-feature.md) |
| v9.0.1 | Added Cheat Indicator Hide when Legit option | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.1 | Added Catppuccin theme | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.1 | Added Interface tab to disable Mega Hack UI changes | mapped | [ui-addons.md](ui-addons.md) | tag 496 (0x1F0, runtime-assigned Map B); `FUN_1801985F0` hooks `EndLevelLayer::customSetup` (RVA 0x132E50) and adds "v9.1.3" label unless tag is enabled. See ui-addons.md. |
| v9.0.1 | Added Smart StartPos toggles (Gamemode, Speed, Size, Mirror) | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--smart-startpos-2026-10-25) | tags 237/238/239/240 (`0xED`..`0xF0`, runtime-assigned) gate the four applications inside the Smart StartPos body. |
| v9.0.0 | Added Iconic (built-in Icon Effects to the Garage) | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added Accurate Percentage | mapped | [best-run-feature.md](best-run-feature.md) | tag 429 (0x1AD, runtime-assigned Map B); `FUN_180165110` hooks `PlayLayer::updateProgressbar` (RVA 0x3B23C0) and formats percentage with configurable decimals (tag 0x1AE = 430). See best-run-feature.md. |
| v9.0.0 | Added Advanced Formatting to Statuses | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.0 | Added Allow Pause Buffering | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Auto Kill | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Ball Rotation Bug | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Classic Pulse (pulsing above 60hz) | cluster | [no-pulse-feature.md](no-pulse-feature.md#ruled-out-for-classic-pulse) | Not located: the tag is runtime-assigned (no `HACK_CLASSIC_PULSE` label), no 1/60 or 60.0 pulse constant exists in the image, and the second `updateVisibility` hook is the hitbox pass. The reason it stays cluster is recorded in the note. |
| v9.0.0 | Added Classic Wave Trail (reimplementation of the 2.1 wave) | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Click Between Frames toggle | cluster | [input-hooks.md](input-hooks.md) |
| v9.0.0 | Added Compact Lists | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.0 | Added Confirm Exit (shortcut for the in-game setting) | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added Custom Background (successor to Transparent BG) | mapped | [cosmetic-feature.md](cosmetic-feature.md) | tag 463 (0x1CF, runtime-assigned Map B); `FUN_180186D10` registers listener for tag 0x1D0 (464) and hooks `CCTransitionScene::initWithDuration`. `FUN_180186FC0` calls `FUN_180187030` to hide background when enabled. See cosmetic-feature.md. |
| v9.0.0 | Added EndLayer extras (StartPos, Checkpoints, Noclip) | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Frame Extrapolation | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Force Ice | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Hitbox Multiplier | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.0.0 | Added Jump Hack | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Load Audio to Memory | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v9.0.0 | Added No Fade Out Audio | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v9.0.0 | Added No Glow | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added No Mirror | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added No Orb Ring | mapped | [render-hooks.md](render-hooks.md#evidence---three-runtime-assigned-cosmetic-tags-2026-10-25) | tag 420 (`0x1A4`, runtime-assigned, no static label); registered at `0x1800b1131`, callback `FUN_18016a370` hooks `RingObject::spawnCircle` (RVA 0x4A1DB0), detour `0x18016a3e0` is a bare `ret`. |
| v9.0.0 | Added No Particles Classic (for pre-v9 behaviour) | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added No Portal Circle | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Menu Snow in December | mapped | [cosmetic-feature.md](cosmetic-feature.md) | tag 498 (0x1F2, runtime-assigned Map B); `FUN_180154AD0` checks December via `_time64`, registers snow particle creator `FUN_180154C10` on `MenuLayer::init` and listener `FUN_1801550F0`. See cosmetic-feature.md. |
| v9.0.0 | Added Player on Top | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Pause During Complete | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Shipcopter | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Show Total Attempts | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.0 | Added Show Trajectory | cluster | [show-trajectory-feature.md](show-trajectory-feature.md) |
| v9.0.0 | Added Transition Customiser (with duration) | mapped | [ui-addons.md](ui-addons.md) | tag 449 (`HACK_NO_TRANSITION`); hooks `CCTransitionFade::create` via `FUN_18018B7F0` (registered by `FUN_18018AEB0`). Tags 0x1D6 (470, duration) and 0x1D8 (472, opacity) are runtime-assigned. See ui-addons.md. |
| v9.0.0 | Added Transparent Lists | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.0 | Added Active Only option to Show Hitboxes | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.0.0 | Added Death Object Only option to Show Hitboxes | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.0.0 | Added Noclip Death Viewer | cluster | [noclip-feature.md](noclip-feature.md) |
| v9.0.0 | Added Noclip option to Show Hitboxes on Death | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.0.0 | Added Ignore Speedhack option to Respawn Time | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Trail Cutting Rate option | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Previews | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added Dual Mode customisation | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added Mini Icons in Normal Mode | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added Extra Colour customisation | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.0 | Added Trail customisation | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added Particle customisation | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added Ghost Trail customisation | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added multi-lingual support | mapped | [ui-addons.md](ui-addons.md) | language-key-driven: `FUN_180197DA0` registers `FUN_180197EA0` on `GameManager::setGameVariable` (RVA 0x182EB0) which toggles hack 0x1ED (493) on key "0025" and sets config 0x1EE (494) on key "0170". Listener `FUN_180197F40` updates UI. See ui-addons.md. |
| v9.0.0 | Added extensions v3 support | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added themes for re-colouring every aspect of the interface (not just accents) | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added cursor showing/hiding opening the menu in-level | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added Indicator Dots (togglable!) | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added a floating persistent button for activation | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added NodeIDs dependency | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.3-alpha.1 | Added Screenshotting | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.3-alpha.1 | Added Wave Trail on Death | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.3-alpha.1 | Added Best Run Accurate Percentage toggle | cluster | [best-run-feature.md](best-run-feature.md) |
| v8.0.3-alpha.1 | Added Glow to Icon Effects | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.2-alpha.1 | Added Auto Music Sync | mapped | [audio-sync-feature.md](audio-sync-feature.md#evidence--auto-music-sync-2026-10-26) | tag 326 (`0x146`, `HACK_AUTO_MUSIC_SYNC`, store `0x180062AFF`); registered at `0x1800b1eb6`, callback `0x1801751e0` hooks `PlayLayer::postUpdate` (RVA 0x3B4BA0) with `0x180175250`, which after six gates calls `0x180175310`: read the music channel position via `FMOD::Channel::getPosition`, compare against the player position plus music offset and the 40 ms constant, and `setPosition` when the drift exceeds tag 327. |
| v8.0.2-alpha.1 | Added Fast Chests | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25) | tag 461 (`0x1CD`, `HACK_FAST_CHESTS`); registered in `FUN_1800afae0` at `0x1800b300c`, callback `FUN_180187820` hooks `RewardUnlockLayer::init` (RVA 0x3CDDF0) and `playRewardEffect`/`showCloseButton`; detours set flag `0x18080C8E0` around the original call. |
| v8.0.2-alpha.1 | Added No Portal Lightning | mapped | [render-hooks.md](render-hooks.md) | tag 416 (`HACK_NO_PORTAL_LIGHTNING`, 0x1A0); registered in `FUN_1800afae0` at `0x1800b1296`, callback `FUN_180171170` (hooks `GJBaseGameLayer::lightningFlash`). See no-portal-lightning-feature.md. |
| v8.0.2-alpha.1 | Added Main Level Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.2-alpha.1 | Added Disable Keybinds in Editor | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.2-alpha.1 | Added Best Run option to only show current startpos runs | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.1-alpha.9 | Added Stop Triggers on Death | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--stop-triggers-on-death-2026-10-25) | tag 415 (`0x19F`, `HACK_STOP_TRIGGERS_ON_DEATH`); registered at `0x1800b1af4`, callback `FUN_180172de0` hooks `GJBaseGameLayer::update` (RVA 0x237850); detour `0x180172e50` tail-jumps to the `getModifiedDelta` gateway (RVA 0x2377B0) when `[host_0x17B4A0+0x208] == this` and a player slot at `layer+0xDA0`/`0xDA8` carries byte `+0x9C0`, otherwise to the cached original. |
| v8.0.1-alpha.9 | Added Auto Pickup Coins | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--the-two-coin-collection-hooks-2026-10-25) | tag 320 (`0x140`, `HACK_AUTO_PICKUP_COINS`); registered at `0x1800b1f2d`, callback `FUN_1801754c0` hooks `PlayLayer::resetLevel` (RVA 0x3B8EB0); detour `0x180175530` calls the original first, then calls host RVA `0x2161b0` for each candidate whose tag-321 flag is non-zero. |
| v8.0.1-alpha.9 | Added Auto Song Downloader | mapped | [ui-addons.md](ui-addons.md#evidence--auto-song-downloader-2026-10-25) | tag 323 (`0x143`, `HACK_AUTO_SONG_DOWNLOAD`); registered at `0x1800b1fa4`, callback `FUN_180175640` hooks `LevelInfoLayer::init` (RVA 0x2F7340); detour `0x1801756b0` runs the cached original first, then, when `[layer+0x2D0]` and its `+0x180` sub-object's vtable slot `+0x148` both pass, runs `CCCallFuncO::create(subObject, <host RVA 0xCABC0>, subObject)` on that sub-object. |
| v8.0.1-alpha.8 | Added Accurate Save | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.1-alpha.7 | Added Replay with brand new interface | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.1-alpha.7 | Added Replay auto-save | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.1-alpha.7 | Added Safe Mode Popup | mapped | [safe-mode-feature.md](safe-mode-feature.md#evidence--the-play-button-popup-2026-10-25) | tag 483 (`0x1E3`, `HACK_SAFE_MODE_POPUP`); three code-cave detours on `LevelInfoLayer::onPlay` (`0x18019a4d0`), `EditLevelLayer::onPlay` (`0x18019a520`) and `LevelPage::onPlay` (`0x18019a570`) forward to the cached original unless suppression byte `0x18080cbb0` is 0 and `0x18019b350` (`areCheatsEnabled`) returns true, in which case they jump to `0x18019a5c0`, which builds the "Safe Mode" / "You have cheats enabled…" alert with Play and Cancel. |
| v8.0.1-alpha.5 | Added Replay (beta) | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.1-alpha.5 | Added Capture | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.1-alpha.5 | Added Attempt Status | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.1-alpha.5 | Added Menu Keybind Fallthrough option | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.1-alpha.5 | Added menu-wide pasting | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.1-alpha.5 | Added an improved Practice Bug Fix | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--practice-bug-fix-2026-10-25) | same tag 317 as the v8.0.0-alpha10b row; this build has one `HACK_PRACTICE_BUG_FIX` label and one settings key, so the improved revision ships as the same detour. |
| v8.0.1-alpha.4 | Added No Wave Trail Behind | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 414 (`HACK_NO_WAVE_TRAIL_BEHIND`, 0x19E); registered in `FUN_1800afae0` at `0x1800b182a`, callback `FUN_180172280` (hooks `PlayerObject::activateStreak`). See no-wave-trail-behind-feature.md. |
| v8.0.1-alpha.3 | Added Icon Randomiser with editor support | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.3 | Added Icon Effects with editor support | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.3 | Added Same Dual Colour | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.2 | Added Pitch Shifter | mapped | [audio-sync-feature.md](audio-sync-feature.md) | tag 459 (`HACK_PITCH_SHIFTER`); listener `FUN_18018A740` (tag 0x1CC = 460) calls `FUN_18018A800` → `FUN_18020DA00` (applies pitch to FMOD channels 0–4). See audio-sync-feature.md. |
| v8.0.0-alpha13b | Added Main Menu Gameplay | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--main-menu-gameplay-2026-10-25) | tag 457 (`0x1C9`, `HACK_MAIN_MENU_PLAY`, static map `0x180063529`); registered at `0x1800b308d`, callback `FUN_180189260` hooks `MenuGameLayer::tryJump` (0x331F70), `update` (0x332240) and `ccTouchBegan` (0x333600), driven by flag `0x18080C920`. |
| v8.0.0-alpha13b | Added TPS Bypass | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 490 (`HACK` TPS Bypass); hooks `GJBaseGameLayer::getModifiedDelta` (RVA 0x2377B0) via `FUN_180196CA0` (registered by `FUN_180196C30`). Tags 0x1E6 (486, TPS value) and 0x1E7 (487, sub-toggle) are runtime-assigned. See gameplay-hooks.md. |
| v8.0.0-alpha13b | Added Hitbox Trail | mapped | [show-hitboxes-feature.md](show-hitboxes-feature.md) | tag 360 (0x168, runtime-assigned Map B) controls trail overlay draw node 0x1E6813C visibility via `FUN_1801589F0`. Main trail draw is `FUN_1801809A0` (tag 224). See show-hitboxes-feature.md. |
| v8.0.0-alpha13b | Added No Pulse | mapped | [no-pulse-feature.md](no-pulse-feature.md#evidence--no-pulse-detour-body-2026-10-26) | tag 413 (`HACK_NO_PULSE`, 0x19D); registered in `FUN_1800afae0` at `0x1800b1384`, callback `FUN_1801713e0`; detour `0x180171450` pins three pulse floats to 0.5f before the cached original runs. |
| v8.0.0-alpha13b | Added Real Time to Lock Delta | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha12 | Added Replay | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.0-alpha12 | Added Replay for Platformer mode | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.0-alpha12 | Added Tooltips | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha12 | Added Demonlist, Challengelist, Speedrun & Leaderboard Rulesets | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha12 | Added Instant Complete | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25) | tag 89 (`0x59`, `HACK_INSTANT_COMPLETE`); registered at `0x1800b25b9`, callback `FUN_1801789e0` hooks `PlayLayer::postUpdate` (0x3B4BA0), `levelComplete` (0x3A7A80) and `updateTimeWarp` (0x3AB9A0); the `levelComplete` detour sets flag `0x18080C8C8` around the original call. |
| v8.0.0-alpha12 | Added Classic Borderless Fullscreen | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha12 | Added Physics Sync | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha12 | Added Confirm Reset | mapped | [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25) | tag 313 (`0x139`, `HACK_CONFIRM_RESET`); registered at `0x1800b22ea`, callback `FUN_1801774b0` hooks `PauseLayer::onRestart` (RVA 0x37E410); detour `0x180177520` shows a 0x240-byte dialog unless guard `0x18080C638` is set, then tail-jumps the handler `0x1801A3150`. |
| v8.0.0-alpha12 | Added Confirm Full-Reset | mapped | [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25) | tag 314 (`0x13A`, `HACK_CONFIRM_FULL_RESET`); registered at `0x1800b2361`, callback `FUN_180177680` hooks `PauseLayer::onRestartFull` (RVA 0x37E4B0); detour `0x1801776f0` shares guard `0x18080C638` and handler `0x1801A3220`. |
| v8.0.0-alpha12 | Added Confirm Practice | mapped | [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25) | tag 311 (`0x137`, `HACK_CONFIRM_PRACTICE`); registered at `0x1800b2273`, callback `FUN_1801772e0` hooks `PauseLayer::onPracticeMode` (RVA 0x37E1C0); detour `0x180177350` shares guard `0x18080C638` and handler `0x1801A3080`. |
| v8.0.0-alpha12 | Added Hide Player | mapped | [render-hooks.md](render-hooks.md#evidence---hide-player-verified-2026-10-25) | tag 386 (`0x182`, `HACK_HIDE_PLAYER`); callback `FUN_180167a60` hooks `PlayerObject::toggleVisibility` (RVA 0x38B440), detour `0x180167b10` forces `edx=0` before `jmp 0x1801a9b70`. |
| v8.0.0-alpha12 | Added Hide Pause Button | mapped | [render-hooks.md](render-hooks.md#evidence--hide-pause-button-and-hide-pause-menu-2026-10-25) | tag 385 (`0x181`, `HACK_HIDE_PAUSE_BUTTON`); registered at `0x1800b0b26`, callback `FUN_180167d10` hooks `UILayer::init` (RVA 0x4CCB00); detour calls `0x1801e9e10`, then hides `[UILayer+0x238]` through vtable slot `+0x140` with `edx=0`. |
| v8.0.0-alpha12 | Added Thread Priority | mapped | [render-hooks.md](render-hooks.md) | tag 454 (`HACK_THREAD_PRIORITY`); `FUN_18018ADF0`/`FUN_18018AE50` call `SetThreadPriority` when enabled. Tag 0x1C7 (455) is the priority level. See render-hooks.md. |
| v8.0.0-alpha12 | Added Switch After Death to StartPos Switcher | mapped | [startpos-switcher-feature.md](startpos-switcher-feature.md#evidence--startpos-switcher-family-2026-10-26) | tag 168 (`0xA8`, static map entry 17, store `0x180065055`); `0x180185CE0` parks the choice in `0x18080C878` with flag `0x18080C880`, the reset hook `0x180184A80` applies it, and the `PlayLayer::init` detour `0x1801846C0` clears the flag. |
| v8.0.0-alpha12 | Added support for NodeIDs (Geode) | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha10b | Added Wave Trail Fix | mapped | [render-hooks.md](render-hooks.md#evidence--wave-trail-fix-2026-10-25) | tag 411 (`0x19B`, `HACK_WAVE_TRAIL_FIX`); registered at `0x1800b094a`, callback `FUN_180166a80` hooks `HardStreak::updateStroke` (RVA 0x2B8EF0); detour `0x180166af0` never calls the original: it clears the `CCDrawNode`, then draws a 6.0x-width ribbon and a 2.0x white ribbon at 0.65x opacity through helper `0x180166c40`. |
| v8.0.0-alpha10b | Added Smooth Editor Trail | mapped | [creator-feature.md](creator-feature.md#evidence--smooth-editor-trail-2026-10-25) | tag 55 (`0x37`, `HACK_SMOOTH_EDITOR_TRAIL`, static map `0x180064222`); registered at `0x1800b058d`, callback `FUN_180163ec0` hooks `LevelEditorLayer::postUpdate` (RVA 0x2E0F40); detour `0x180163f30` stores `0x3D088889` (1/30 s) at `[layer+0x3898]` then jumps to the original. |
| v8.0.0-alpha10b | Added Screenshots | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha10b | Added Practice Bug Fix | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--practice-bug-fix-2026-10-25) | tag 317 (`0x13D`, `HACK_PRACTICE_BUG_FIX`, static map `0x180063CD1`); post-hook on `PlayLayer::resetLevel` re-applies the newest checkpoint's saved player state. |
| v8.0.0-alpha10b | Added custom click/move keybinds | cluster | [input-hooks.md](input-hooks.md) |
| v8.0.0-alpha10b | Added Best Run | cluster | [best-run-feature.md](best-run-feature.md) |
| v8.0.0-alpha10b | Added Collect Coins in Practice | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--the-two-coin-collection-hooks-2026-10-25) | tag 297 (`0x129`, `HACK_COLLECT_COINS_IN_PRACTICE`); registered at `0x1800b2185`, callback `FUN_180176ea0` hooks `GJBaseGameLayer::collisionCheckObjects` (RVA 0x214960); detour `0x180176f10` keeps type ids `0x1F`/`0x16` with `+0x3d2` clear that pass `0x1802045d0`, calls host RVAs `0x2161b0` and `0x216090`, then tail-jumps to the original. |
| v8.0.0-alpha10b | Added Checkpoint Limit Bypass | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25) | tag 298 (`0x12A`, `HACK_CHECKPOINT_LIMIT_BYPASS`); registered at `0x1800b2092`, callback `FUN_180176250` hooks `PlayLayer::storeCheckpoint` (RVA 0x3B74A0), detour `0x1801762c0` replaces the second argument with `[checkpoint+0xE58]` before tail-jumping to the original. |
| v8.0.0-alpha10b | Added No "Do Not" Flip | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 412 (`HACK_NO_DO_NOT_FLIP`, 0x19C); registered in `FUN_1800afae0` at `0x1800b0e67`, callback `FUN_1801696e0` (hooks `EndLevelLayer::customSetup` + `PlayLayer::setupHasCompleted`). See no-do-not-flip-feature.md. |
| v8.0.0-alpha10b | Added Meta Status | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha10b | Added Toolbox Button Bypass | mapped | [ui-addons.md](ui-addons.md#evidence--toolbox-button-bypass-2026-10-25) | tag 47 (`0x02F`, `HACK_TOOLBOX_BUTTON_BYPASS`, static map `0x1800644EA`); two code-cave replacements for `EditorOptionsLayer::onButtonRows` (RVA 0x291A00) and `onButtonsPerRow` (0x291970) drop the upper bound on the editor's button counts. |
| v8.0.0-alpha10b | Added Place Over | mapped | [ui-addons.md](ui-addons.md#evidence--confirmation-dialogs-and-place-over-2026-10-25) | tag 51 (`0x33`, `HACK_PLACE_OVER`); registered at `0x1800b0511`, callback `FUN_180163000` hooks `LevelEditorLayer::typeExistsAtPosition` (RVA 0x2D4A50); detour `0x180163070` is `xor eax,eax; ret`, so the editor never sees a position as occupied. |
| v8.0.0-alpha9b | Added auto update to Geode builds | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Auto Deafen | mapped | [audio-sync-feature.md](audio-sync-feature.md) | tag 279 (`HACK_AUTO_DEAFEN`, 0x117); registered in `FUN_1800afae0` at `0x1800b1dc8`, callback `FUN_180174a80` (hooks `PlayLayer::resetLevel`). See auto-deafen-feature.md. |
| v8.0.0-alpha9 | Added Show Triggers | mapped | [render-hooks.md](render-hooks.md#evidence--show-triggers-2026-10-25) | tag 286 (`0x11E`, `HACK_SHOW_TRIGGERS`, static map `0x1800640F7`); registered at `0x1800b2cc6`, callback `FUN_180183790` hooks `PlayLayer::addObject` (0x3ADD70), `GJBaseGameLayer::addToSection` (0x226500) and `GameObject::customSetup` (0x190E20); the customSetup detour accepts object IDs 0xB53..0xBD9. |
| v8.0.0-alpha9 | Added Jumpscare | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--jumpscare-2026-10-25) | tag 291 (`0x123`, `HACK_JUMPSCARE`, static map `0x180063420`); hook on `PlayLayer::destroyPlayer` (RVA 0x3B39D0) adds a `jumpscare.png` overlay on `+0x3540` at z-order 100 and plays `jumpscare.mp3` when the tags 292-295 gate passes. |
| v8.0.0-alpha9 | Added Replay Last Checkpoint | mapped | tag 316 (`0x13C`, `HACK_REPLAY_LAST_CHECKPOINT`); registered at `0x1800b2a6e`, callback `FUN_18017ecb0` hooks `EndLevelLayer::onReplay` (RVA 0x134E80); detour `0x18017ed20` reroutes to host RVA 0x1354C0 with `edx=0` when the second argument is null and `[playlayer+0x31f0] == 1`, otherwise to the cached original. See [gameplay-hooks.md](gameplay-hooks.md#evidence--replay-last-checkpoint-2026-10-25). |
| v8.0.0-alpha9 | Added 0% Practice Complete | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 296 (`HACK_0_PRACTICE_COMPLETE`, static map entry #0); read via `hackpro::Config` vtable (`FUN_1800917B0` at 0x180544FF0). No gameplay code reads it directly; accessed through exported `isHackEnabled` API. See gameplay-hooks.md. |
| v8.0.0-alpha9 | Added Uncomplete Level | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Trigger Value Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Hide Pause Menu | mapped | [render-hooks.md](render-hooks.md#evidence--hide-pause-button-and-hide-pause-menu-2026-10-25) | tag 410 (`0x19A`, `HACK_HIDE_PAUSE_MENU`); registered at `0x1800b0b9d`, callback `FUN_180167fa0` hooks `PauseLayer::customSetup` (RVA 0x37C870); detour calls `0x1801a2c30` then tail-jumps vtable slot `+0x140` with `edx=0`. |
| v8.0.0-alpha9 | Added Hide UI | mapped | [render-hooks.md](render-hooks.md#evidence--hide-ui-2026-10-25) | tag 53 (`0x35`, `HACK_HIDE_UI`, static map `0x180063372`); registered at `0x1800b0330`, callback `FUN_18015fa70` hooks `EditorUI::init` (RVA 0xDF390); detour `0x18015fb20` calls vtable `+0x140` with `edx=0` (hide) when its gate returns true. |
| v8.0.0-alpha9 | Added Restart, Practice & folder shortcuts | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha8 | Added Show Hitboxes | mapped | [show-hitboxes-feature.md](show-hitboxes-feature.md) | tag 192 (`HACK_SHOW_HITBOXES`); gate `FUN_180203EE0`, draw `FUN_180203F60`→`FUN_180204000`/`FUN_180204150`; per-object draw `FUN_180203070`. See show-hitboxes-feature.md. |
| v8.0.0-alpha8 | Added Show Hitboxes on Death | mapped | [show-hitboxes-feature.md](show-hitboxes-feature.md) | tag 216 (`HACK_SHOW_HITBOXES_ON_DEATH`); gate `FUN_180203EE0` branch 2, death-object draw via `FUN_180204000` (uses `DAT_18080CB80` from `destroyPlayer` callback). See show-hitboxes-feature.md. |
| v8.0.0-alpha8 | Added Wave Pulse Size | mapped | [render-hooks.md](render-hooks.md) | tag 407 (`HACK_WAVE_PULSE_SIZE`, 0x197); registered in `FUN_1800afae0` at `0x1800b1c59`, callback `FUN_180173920` (hooks `PlayerObject::update`). See wave-pulse-size-feature.md. |
| v8.0.0-alpha8 | Added No (C) Mark | mapped | [render-hooks.md](render-hooks.md) | tag 44 (`HACK_NO_C_MARK`, 0x02C); registered in `FUN_1800afae0` at `0x1800b049a`, callback `FUN_180162d20` (hooks `ShareLevelLayer::onShare`). See no-c-mark-feature.md. |
| v8.0.0-alpha8 | Added No Shake | mapped | [no-shake-feature.md](no-shake-feature.md) | tag 409 (`HACK_NO_SHAKE`, 0x199); registered in `FUN_1800afae0` at `0x1800b1560`, callback `FUN_1801718C0` (hooks `PlayLayer::init`, RVA 0x3A5B00, detour 0x180171930). |
| v8.0.0-alpha8 | Added Clock | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha7 | Added Classic Particles | mapped | [render-hooks.md](render-hooks.md) | tag 390 (`HACK_CLASSIC_PARTICLES`, 0x186); registered in `FUN_1800afae0` at `0x1800b085c`, callback `FUN_180166270` (hooks `cocos2d::CCParticleSystem::loadScaledDefaults`). See classic-particles-feature.md. |
| v8.0.0-alpha7 | Added Noclip Accuracy | mapped | [noclip-feature.md](noclip-feature.md) | tag 497 (0x1F1, runtime-assigned Map B); `FUN_180152010` hooks `EndLevelLayer::showLayer` (RVA 0x134CA0) and draws "Noclip: {:.2f}%" unless tag enabled. Also reads `HACK_NOCLIP` (tag 72). See noclip-feature.md. |
| v8.0.0-alpha7 | Added Noclip Limits | mapped | [noclip-feature.md](noclip-feature.md) | tag 83 (`HACK_NOCLIP_LIMITS`); `FUN_18017A290` implements accuracy limit (tag 0x55/0x54) and death limit (tag 0x57/0x56) gating tint overlay. See noclip-feature.md. |
| v8.0.0-alpha7 | Added Cheat Indicator with breakdown | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha7 | Added FPS & CPS Statuses | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha7 | Added Message, Percentage & Level Time Statuses | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha6 | Added Show Layout | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--show-layout-2026-10-25) | tag 260 (`0x104`, `HACK_SHOW_LAYOUT`, static map `0x180064095`); registered at `0x1800b2b5c`, callback `FUN_18017f0f0` installs six hooks (`resetLevel`, `applyEnterEffect`, `applyCustomEnterEffect`, `triggerObject`, `addGlow`, `saveActiveColors`); the resetLevel detour reads Map A tags 262 and 263. |
| v8.0.0-alpha6 | Added Endscreen Hider | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha5 | Added Smart StartPos | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--smart-startpos-2026-10-25) | tag 235 (`0x0EB`, `HACK_SMART_STARTPOS`, static map `0x1800641B8`); replaces `PlayLayer::resetLevel` (RVA 0x3B8EB0) with a post-hook that re-applies the nearest preceding gamemode/speed/size/mirror object. |
| v8.0.0-alpha5 | Added Respawn Time modifier | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--respawn-time-modifier-2026-10-25) | tag 245 (`0x0F5`, `HACK_RESPAWN_TIME`); registered at `0x1800b2ae5`, callback `FUN_18017ef50` hooks `PlayLayer::destroyPlayer` (RVA 0x3B39D0); detour reads the double at tag 246, divides by 1000.0, and hands seconds to helper `0x18017f000`, which clamps to 2^-23 and multiplies by tag 501 when tags 502 and 247 are on. |
| v8.0.0-alpha5 | Added Confirm Exit | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha5 | Added Allow Low Volume | mapped | [gameplay-hooks.md](gameplay-hooks.md#evidence--four-labelled-tags-found-by-the-full-section-scan-2026-10-25) | tag 451 (`0x1C3`, `HACK_ALLOW_LOW_VOLUME`); registered at `0x1800b2ea7`, callback `FUN_180186100` hooks `PauseLayer`/`OptionsLayer` music and sfx slider callbacks (RVAs 0x37E130, 0x372A90, 0x372990), each detour recomputes the float and tail-jumps to the volume setter `0x180129980`. |
| v8.0.0-alpha5 | Added No Particles | mapped | [render-hooks.md](render-hooks.md) | tag 402 (`HACK_NO_PARTICLES`, 0x192); registered in `FUN_1800afae0` at `0x1800b11a8`, callback `FUN_1801703b0`. See no-particles-feature.md. |
| v8.0.0-alpha5 | Added No Wave Trail | mapped | [render-hooks.md](render-hooks.md) | tag 397 (`HACK_NO_WAVE_TRAIL`, 0x18D); registered in `FUN_1800afae0` at `0x1800b17b3`, callback `FUN_1801721f0` (hooks `HardStreak::updateStroke`). See no-wave-trail-feature.md. |
| v8.0.0-alpha5 | Added Solid Wave Trail | mapped | [cosmetic-feature.md](cosmetic-feature.md) | tag 396 (`HACK_SOLID_WAVE_TRAIL`); blend-func toggle in `FUN_1801EECA0` (solid 0x30300000001 vs blended 0x100000302); registered in `FUN_1800afae0` at `0x1800b1a06`, callback `FUN_1801728c0` hooks `PlayerObject::setupStreak` (RVA 0x3887C0) and `PlayerObject::updateStreakBlend` (0x388CA0). See cosmetic-feature.md and [hook-registrations.md](hook-registrations.md#worked-example-promoting-a-candidate-row-to-verified). |
| v8.0.0-alpha5 | Added No Respawn Flash | mapped | [render-hooks.md](render-hooks.md) | tag 398 (`HACK_NO_RESPAWN_FLASH`, 0x18E); registered in `FUN_1800afae0` at `0x1800b13fb`, callback `FUN_1801714c0` (hooks `PlayerObject::playSpawnEffect`). See no-respawn-flash-feature.md. |
| v8.0.0-alpha5 | Added Force StartPos Reset Camera | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added StartPos Switcher keybind options | mapped | [startpos-switcher-feature.md](startpos-switcher-feature.md#evidence--startpos-switcher-family-2026-10-26) | tags 131 (`0x83`) and 132 (`0x84`) are read with `0x18002EE40` in `0x180185070` and compared against the pressed key before next/previous dispatch. |
| v8.0.0-alpha5 | Added StartPos Switcher interface | mapped | [startpos-switcher-feature.md](startpos-switcher-feature.md#evidence--startpos-switcher-family-2026-10-26) | installed unconditionally by `0x180184490` (called at `0x1800B2DEE`): hooks `PlayLayer::init`, `PlayLayer::setupHasCompleted` and `UILayer::init`; constructor `0x180184710` builds the `0x1E8`-byte node with class pointer `0x180578CF8`. |
| v8.0.0-alpha5 | Added Hide Interface to StartPos Switcher | mapped | [startpos-switcher-feature.md](startpos-switcher-feature.md#evidence--startpos-switcher-family-2026-10-26) | tag 169 (`0xA9`, static map entry 4, store `0x180064B32`); listener `0x1801848A0` installed from `0x1801845B0` refreshes `0x180184BA0`, which hides `+0x1C0`/`+0x1C8` and shows `+0x1D0` when the tag is set. |
| v8.0.0-alpha5 | Added Inject DLL | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha4 | Added Safe Mode | mapped | [safe-mode-feature.md](safe-mode-feature.md) |
| v8.0.0-alpha4 | Added Auto Safe Mode | mapped | [safe-mode-feature.md](safe-mode-feature.md) |
| v8.0.0-alpha4 | Added Platformer Practice | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha4 | Added Trail Cutting | mapped | [render-hooks.md](render-hooks.md) | tag 392 (`HACK_TRAIL_CUTTING`, 0x188); registered in `FUN_1800afae0` at `0x1800b1be2`, callback `FUN_1801732c0` (hooks `cocos2d::CCMotionStreak::initWithFade`). See trail-cutting-feature.md. |
| v8.0.0-alpha4 | Added Zero Delay | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha4 | Added Text Length Bypass | mapped | [ui-addons.md](ui-addons.md#evidence--text-length-bypass-2026-10-25) | tag 37 (`0x25`, `HACK_TEXT_LENGTH`, static map `0x180064420`); registered at `0x1800afe13`, callback `FUN_18015d6d0` hooks `CCTextInputNode::onTextFieldAttachWithIME` (RVA 0x508F0); detour `0x18015d740` calls `0x1801e6d20(rcx-0x1A0, 0x7FFFFFFF)` then tail-jumps the original. |
| v8.0.0-alpha4 | Added Slider Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha4 | Added No Trail | mapped | [render-hooks.md](render-hooks.md) | tag 395 (`HACK_NO_TRAIL`, 0x18B); registered in `FUN_1800afae0` at `0x1800b16c5`, callback `FUN_180171db0` (hooks `cocos2d::CCMotionStreak::draw`). See no-trail-feature.md. |
| v8.0.0-alpha4 | Added No Wave Pulse | mapped | [render-hooks.md](render-hooks.md) | tag 394 (`HACK_NO_WAVE_PULSE`, 0x18A); registered in `FUN_1800afae0` at `0x1800b173c`, callback `FUN_180172160` (hooks `PlayerObject::update`). See no-wave-pulse-feature.md. |
| v8.0.0-alpha4 | Added Disable Progress to Noclip | mapped | [noclip-feature.md](noclip-feature.md) |
| v8.0.0 (initial release) | Added Profiles | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0 (initial release) | Added Mouse 4 & 5 Support to Keybinds | cluster | [input-hooks.md](input-hooks.md) |
| v8.0.0 (initial release) | Added Controller Support to Keybinds | cluster | [input-hooks.md](input-hooks.md) |
| v8.0.0 (initial release) | Added Alt-Show/Hide Menu Hotkey | cluster | [input-hooks.md](input-hooks.md) |
| v8.0.0 (initial release) | Added Rulesets | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0 (initial release) | Added colour customisation | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v8.0.0 (initial release) | Added blurred BG effect (fullscreen/tabs-only/none) | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v7.1.1 (Full) | Added Invisible Dual Fix | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1.1 (Full) | Added Magic Screenshots | cluster | [ui-addons.md](ui-addons.md) |
| v7.1.1 (Full) | Added Only Death Object option for Hitboxes on Death | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v7.1.1 (Full) | Added Quick Chests | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1.1 (Full) | Added Random Menu Music | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v7.1.1 (Full) | Added Random SFX | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v7.1.1 (Full) | Added Secure Servers | cluster | [ui-addons.md](ui-addons.md) |
| v7.1.1 (Full) | Added Text Cursor | cluster | [ui-addons.md](ui-addons.md) |
| v7.1.1 (Full) | Added Trigger conditions for Hitboxes on Death | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1.1 (Full) | Added Extensions Preload Folder (textureldr fix) | cluster | [ui-addons.md](ui-addons.md) |
| v7.1 (Full) | Added Mega Hack Replay | cluster | [replay-feature.md](replay-feature.md) |
| v7.1 (Full) | Added Anti Crash-Trigger | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Auto Backup | cluster | [ui-addons.md](ui-addons.md) |
| v7.1 (Full) | Added Auto Kill At % | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Auto No Auto-Retry | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Dash Orb Colour Switch | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v7.1 (Full) | Added Editor Hitbox Bug Fix | cluster | [creator-feature.md](creator-feature.md) |
| v7.1 (Full) | Added Editor Hitbox Only on Select | cluster | [creator-feature.md](creator-feature.md) |
| v7.1 (Full) | Added Editor Support for Smart StartPos | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Fast Practice Respawn Time Modifier | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Force Objects Invisible | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Frame Stepper | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 302 (`HACK_FRAME_STEPPER`); registrar `FUN_180173FD0` toggles two UILayer hooks (`onPause` RVA 0x4CDF50, `handleKeypress` RVA 0x4CD830). Tag 357 (0x165, runtime-assigned Map B) is an alternate enable. See gameplay-hooks.md. |
| v7.1 (Full) | Added Hide Checkpoints | cluster | [render-hooks.md](render-hooks.md) |
| v7.1 (Full) | Added Hide Level Complete Screen | cluster | [render-hooks.md](render-hooks.md) |
| v7.1 (Full) | Added Icon Randomiser Unlocked Icons Option | cluster | [iconic-feature.md](iconic-feature.md) |
| v7.1 (Full) | Added Keep Wave Trail on Death | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Load Failed Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Lock Delta | cluster | [render-hooks.md](render-hooks.md) |
| v7.1 (Full) | Added Max CPS Option to Labels | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.1 (Full) | Added Meta (Player Data) Label | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.1 (Full) | Added mirror P1/P2 Inputs | cluster | [input-hooks.md](input-hooks.md) |
| v7.1 (Full) | Added No Centre Preview Line | cluster | [render-hooks.md](render-hooks.md) |
| v7.1 (Full) | Added No Dash Trail | cluster | [render-hooks.md](render-hooks.md) |
| v7.1 (Full) | Added No Death x to editor | cluster | [render-hooks.md](render-hooks.md) |
| v7.1 (Full) | Added No Effect Circle | cluster | [render-hooks.md](render-hooks.md) |
| v7.1 (Full) | Added No Menu Music | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v7.1 (Full) | Added No Robot Fire | mapped | [render-hooks.md](render-hooks.md) | tag 440 (0x1B8, runtime-assigned); registered in `FUN_1800afae0` at `0x1800b1472`, callback `FUN_180171550` (hooks `PlayerObject::toggleRobotMode`). See no-robot-swing-fire-feature.md. |
| v7.1 (Full) | Added Noclip for P1/P2 | cluster | [noclip-feature.md](noclip-feature.md) |
| v7.1 (Full) | Added Pause Countdown | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Play Song to Level page | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.1 (Full) | Added Reverse Input | cluster | [input-hooks.md](input-hooks.md) |
| v7.1 (Full) | Added Screen Recorder | cluster | [ui-addons.md](ui-addons.md) |
| v7.1 (Full) | Added Screenshot to clipboard setting | cluster | [ui-addons.md](ui-addons.md) |
| v7.1 (Full) | Added Status Hide All | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.1 (Full) | Added Swift Click to Shortcuts | cluster | [input-hooks.md](input-hooks.md) |
| v7.1 (Full) | Added Switch P1/P2 Inputs | cluster | [input-hooks.md](input-hooks.md) |
| v7.1 (Full) | Added TPS bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added Favourites | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Screenshotter | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Extension Load Order & Toggling | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added 0% Practice Mode Completion | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added Accurate Percentage Decimal Places | cluster | [best-run-feature.md](best-run-feature.md) |
| v7.0 (Full) | Added Auto Deafen Undeafen Percentage | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v7.0 (Full) | Added Challenge List | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Custom Levels on Launch | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Demon List Roulette | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Detailed Level Complete Screens | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Discord Rich Presence Difficulty & Best % | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Display Resolution Setter | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added Display Smooth Fix, Force Smooth Fix & LDM | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added Editor Anti-Kick | cluster | [creator-feature.md](creator-feature.md) |
| v7.0 (Full) | Added Enter for Level Search | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Freeze Attempts | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added Force Don't Enter | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added Force Don't Fade | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added Hide Attempts in Practice | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added Granular Scaling to UI | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added Icon Custom Gradients | cluster | [iconic-feature.md](iconic-feature.md) |
| v7.0 (Full) | Added Icon Level ColourID Customisation | cluster | [iconic-feature.md](iconic-feature.md) |
| v7.0 (Full) | Added Icon Particle Customisation | cluster | [iconic-feature.md](iconic-feature.md) |
| v7.0 (Full) | Added Icon Reverse Music Flash | cluster | [iconic-feature.md](iconic-feature.md) |
| v7.0 (Full) | Added Icon Stealer to Profiles | cluster | [iconic-feature.md](iconic-feature.md) |
| v7.0 (Full) | Added Icon Swap Main & Secondary Colours | cluster | [iconic-feature.md](iconic-feature.md) |
| v7.0 (Full) | Added Jump Keybind Customisation | cluster | [input-hooks.md](input-hooks.md) |
| v7.0 (Full) | Added Level Status Attempts | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.0 (Full) | Added Level Status Clock | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.0 (Full) | Added Level Status Custom Message | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.0 (Full) | Added Level Status Jumps | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.0 (Full) | Added Level Status Only Total Clicks | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.0 (Full) | Added Level Status Percentage | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.0 (Full) | Added Level Status Session Time | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v7.0 (Full) | Added No Editor Trail | cluster | [creator-feature.md](creator-feature.md) |
| v7.0 (Full) | Added No Hide Player Trigger | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added No Main Trail Behind Wave | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added Noclip Death Counter & Death Limit | cluster | [noclip-feature.md](noclip-feature.md) |
| v7.0 (Full) | Added Pause Void Click Bugfix | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added Pause During Completion | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v7.0 (Full) | Added Pasting to the UI & Extensions | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Percentage Only to Pause Menu | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Replay Last Checkpoint to Endscreen | mapped | tag 316 (`0x13C`, `HACK_REPLAY_LAST_CHECKPOINT`); registered at `0x1800b2a6e`, callback `FUN_18017ecb0` hooks `EndLevelLayer::onReplay` (RVA 0x134E80); detour `0x18017ed20` reroutes to host RVA 0x1354C0 with `edx=0` when the second argument is null and `[playlayer+0x31f0] == 1`, otherwise to the cached original. See [gameplay-hooks.md](gameplay-hooks.md#evidence--replay-last-checkpoint-2026-10-25). |
| v7.0 (Full) | Added Show Password to Copy Level Screen | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Show Level Layout | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added SongID Copy | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added StartPos Switcher | mapped | [startpos-switcher-feature.md](startpos-switcher-feature.md) | tag 130 (`HACK_STARTPOS_SWITCHER`); gate `FUN_180185710` controls next/prev switching via `FUN_1801857F0`/`FUN_180185910`. Tags 0xA6/0xA7 (166/167) are runtime-assigned Map A. See startpos-switcher-feature.md. |
| v7.0 (Full) | Added Transition Customiser | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added UI Transition Time Customisation | cluster | [ui-addons.md](ui-addons.md) |
| v7.0 (Full) | Added Unfocus Mute | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added Unfocused FPS Setter | cluster | [render-hooks.md](render-hooks.md) |
| v7.0 (Full) | Added Wave Trail Bugfix (checkpoints & startpos) | cluster | [render-hooks.md](render-hooks.md) |
| v6.3.0 (Full) | Added Crash Logger | cluster | [ui-addons.md](ui-addons.md) |
| v6.3.0 (Full) | Added Auto Low Detail Mode | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.3.0 (Full) | Added Auto Practice Mode | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.3.0 (Full) | Added Auto-Deafen | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v6.3.0 (Full) | Added Clicks per Second counter | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v6.3.0 (Full) | Added Corrective Music Sync | mapped | [audio-sync-feature.md](audio-sync-feature.md) | tag 502 (0x1F6, runtime-assigned Map B); `FUN_18019C420` registers listener for tag 0x1F7 (503) and hooks `FMOD::ChannelControl::setPitch` + `FMODAudioEngine::setMusicTimeMS`. `FUN_18019C6D0` scales pitch by `DAT_1807F9420`. See audio-sync-feature.md. |
| v6.3.0 (Full) | Added Glow Customisation | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v6.3.0 (Full) | Added Hitbox Viewer | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v6.3.0 (Full) | Added Legacy Mode to Speedhack | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.3.0 (Full) | Added Level Uncompleter | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.3.0 (Full) | Added Music-synced Icons | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v6.3.0 (Full) | Added NoClip Accuracy screen tint | mapped | [noclip-feature.md](noclip-feature.md) | tag 75 (`CHECKBOX_NOCLIP_TINT`); `FUN_18017A290` hooks `PlayLayer::destroyPlayer` (RVA 0x3B39D0) and applies tint overlay via `DAT_18080C710`. Tags 77/78 (player 1/2) and 0x4C (colour) are runtime-assigned. See noclip-feature.md. |
| v6.3.0 (Full) | Added options menu shortcut | cluster | [ui-addons.md](ui-addons.md) |
| v6.3.0 (Full) | Added Playtest Zoom Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.3.0 (Full) | Added Small Grid Size modifier | cluster | [creator-feature.md](creator-feature.md) |
| v6.3.0 (Full) | Added Stable Pulse | cluster | [render-hooks.md](render-hooks.md) |
| v6.3.0 (Full) | Added Trail Customisation | cluster | [iconic-feature.md](iconic-feature.md) |
| v6.3.0 (Full) | Added shortcuts to the Resources & AppData folders | cluster | [ui-addons.md](ui-addons.md) |
| v6.3.0 (Full) | Added position settings for FPS Counter, NoClip Accuracy & Cheat Indicator | cluster | [noclip-feature.md](noclip-feature.md) |
| v6.3.0 (Full) | Added option to disable keybinds in the editor (on by default) | cluster | [input-hooks.md](input-hooks.md) |
| v6.2.1 (Full) | Added Disable All button | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Extensions | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Accurate Leaderboard integration | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Borderless Fullscreen | mapped | [render-hooks.md](render-hooks.md) | tag 492 (0x1EC, runtime-assigned Map B); `FUN_1801968F0` (vtable entry 0x180831A2C) calls `FUN_1801964D0(1)` which toggles window border via `SetWindowLongPtrW` + NOP-patch. See render-hooks.md. |
| v6.2.0 (Full) | Added Coins Show as Uncollected mod | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.2.0 (Full) | Added Col1, Col2, & Wave Trail to RGB Icons | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added ColourID Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Copy LevelID Fix | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Demon List integration | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added FPS Counter | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v6.2.0 (Full) | Added Level Restart button | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Lock Cursor | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added Mini Cube Icon mod | cluster | [iconic-feature.md](iconic-feature.md) |
| v6.2.0 (Full) | Added No Death Effect mod | mapped | [render-hooks.md](render-hooks.md) | tag 399 (`HACK_NO_DEATH_EFFECT`, 0x18F); registered in `FUN_1800afae0` at `0x1800b0df0`, callback `FUN_180169330` (hooks `PlayerObject::playDeathEffect`, RVA 0x37EF10, with a bare-`ret` detour at `0x180169400`). See no-death-effect-feature.md. |
| v6.2.0 (Full) | Added No Force Glow mod | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added No New Best Popup mod | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added No Respawn Flash mod | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added No Wave Pulse mod | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added No Wave Trail mod | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added NoClip Accuracy | cluster | [noclip-feature.md](noclip-feature.md) |
| v6.2.0 (Full) | Added Pastel RGB, Fade & BG Icon Effects | cluster | [iconic-feature.md](iconic-feature.md) |
| v6.2.0 (Full) | Added Practice Mode Fix (by Fig & Andx) | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.2.0 (Full) | Added Practice Pulse mod | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added Practice User-Coin Pickup mod (by WEGFan) | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.2.0 (Full) | Added Refresh Textures button | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Replay Last Checkpoint mod | mapped | tag 316 (`0x13C`, `HACK_REPLAY_LAST_CHECKPOINT`); registered at `0x1800b2a6e`, callback `FUN_18017ecb0` hooks `EndLevelLayer::onReplay` (RVA 0x134E80); detour `0x18017ed20` reroutes to host RVA 0x1354C0 with `edx=0` when the second argument is null and `[playlayer+0x31f0] == 1`, otherwise to the cached original. See [gameplay-hooks.md](gameplay-hooks.md#evidence--replay-last-checkpoint-2026-10-25). |
| v6.2.0 (Full) | Added Rotation Speed & Z Layer modifiers | cluster | [creator-feature.md](creator-feature.md) |
| v6.2.0 (Full) | Added Same Dual Colour mod | cluster | [iconic-feature.md](iconic-feature.md) |
| v6.2.0 (Full) | Added Smart Start Pos | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v6.2.0 (Full) | Added Solid & Dual options to RGB Icons | cluster | [iconic-feature.md](iconic-feature.md) |
| v6.2.0 (Full) | Added Thread Priority Switcher | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added Trail Cut Fix | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added Variable Setter | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Vertical Sync & Fullscreen toggle | cluster | [render-hooks.md](render-hooks.md) |
| v6.1.0 (Full) | Added RGB Icons mod | cluster | [iconic-feature.md](iconic-feature.md) |
| v6.1.0 (Full) | Added Multiscaler | cluster | [render-hooks.md](render-hooks.md) |
| v6.1.0 (Full) | Added Level Password finder | cluster | [ui-addons.md](ui-addons.md) |
| v6.1.0 (Full) | Added DLL Injector | cluster | [ui-addons.md](ui-addons.md) |
| v6.1.0 (Full) | Added No Wave Pulse hack | cluster | [render-hooks.md](render-hooks.md) |
| v6.1.0 (Full) | Added No Flip Portal hack | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.1.0 (Full) | Added Transparent BG & List mods (by WEGFan) | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v6.1.0 (Full) | Added Changelog button | cluster | [ui-addons.md](ui-addons.md) |
| v6.1.0 (Full) | Added 0.625x & 1.25x scaling | cluster | [render-hooks.md](render-hooks.md) |
| v6.0.0 | Added Native Speedhack | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.0.0 | Added keybinding system | cluster | [input-hooks.md](input-hooks.md) |
| v6.0.0 | Added Absolute Positions | cluster | [render-hooks.md](render-hooks.md) |
| v6.0.0 | Added Absolute Scaling | cluster | [render-hooks.md](render-hooks.md) |
| v5.1 | Added gamemode changer | cluster | [gameplay-hooks.md](gameplay-hooks.md) |

## Detour-size triage (2026-10-25)

The remaining `cluster` rows are expensive one at a time, so the first pass is now "which
detours are small enough to settle in one reading?". `scripts/triage_detours.py` answers it:
it takes each detour symbol from `recovered/functions/_hook_sites.tsv`, asks
`scripts/func_bounds.py` for the `.pdata` extent, and for a detour with no unwind record (a
code cave) disassembles until `ret` or an unconditional `jmp`, following conditional branches
through so a two-branch detour is not truncated at its first `jne`.

Result on the baseline artifact: **18 detours are 0xC bytes or shorter**, and every one of
them belongs to a feature that is already `mapped`:

| Detour | Bytes | Target | Owner |
| --- | ---: | --- | --- |
| `0x18015DCD0` / `0x18015DCE0` | 3 | `GameManager::isIconUnlocked` / `isColorUnlocked` | Unlock Icons (tag 32) |
| `0x180163070` | 3 | `LevelEditorLayer::typeExistsAtPosition` | Place Over (tag 51) |
| `0x180172260` | 5 | `HardStreak::updateStroke` | No Wave Trail (tag 397) |
| `0x180167B10` | 7 | `PlayerObject::toggleVisibility` | Hide Player (tag 386) |
| `0x180189F50` / `0x180189F60` | 8 | `CCTransitionFade::create` / `CCTransitionMoveInT::create` | No Transition (tag 449) |
| `0x180169BE0` etc. | 0xC | `PlayLayer::addObject`, `resetLevel`, `init`, `GJBaseGameLayer::addToSection` | Show Triggers, Instant Complete, Fast Chests, other mapped flags |

The one-line bodies are worth recording because they are self-proving: `xor eax, eax; ret`
on `typeExistsAtPosition` cannot mean anything other than "no object of this type exists
here", and `mov al, 1; ret` on the two `GameManager::is*Unlocked` queries cannot mean anything
other than "always unlocked". A target name alone never carried that.

Consequence for planning: the cheap end of the detour space is exhausted. Anything still
`cluster` needs a body read in full, so the next passes should pick rows by feature value
(the changelog wording), not by detour size. `Auto Song Downloader` was the last
0xD-0x18 byte row that also has a changelog row; it was mapped on 2026-10-25 from a post-hook
detour, as were `Safe Mode Popup` and `Replay Last Checkpoint`.

Correction (2026-10-25): the earlier version of this paragraph named `Force Platformer` and
`Default Song Bypass` as the next cheap rows. Both are in the static label map — tags 242
(`0x0F2`, `HACK_FORCE_PLATFORMER`) and 49 (`0x031`, `HACK_DEFAULT_SONG_BYPASS`) — but **neither
has a row in the 389**: no `Added Force Platformer` and no `Added Default Song Bypass` bullet
exists. Mapping them would add entries to "Toggles outside the 389" at best, so they are not
progress against the milestone. The next rows must be chosen from the 303 remaining changelog
bullets.
