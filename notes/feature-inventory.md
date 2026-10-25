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

All 389 rows below were `cluster` at the 2026-10-23 baseline. **Forty-three are now
`mapped`**; the [Mapped rows](#mapped-rows) table below is the authoritative list, with
tag, registration site and hook address for each one.

The 2026-10-24 batch promoted eighteen hook-registered toggles — No (C) Mark, No Death
Effect, No Particles, No Respawn Flash, No Shake, No Portal Lightning, No Pulse, No
Trail, No Wave Trail, No Wave Trail Behind, Wave Pulse Size, No Wave Pulse, Trail
Cutting, No Robot Fire, No Robot/Swing Fire, Classic Particles, No "Do Not" Flip, and
Auto Deafen — each by the same pattern: a `mov edx, <tag>` / `call FUN_18008f910`
registration in the shared initialiser `FUN_1800afae0`, plus a callback that installs
or removes the hook. The other 346 rows remain `cluster`.

A keyword check at the baseline found that **185 of 389** rows point at a note that
never mentions the feature at all, so for those even the cluster assignment is only a
lead. That count predates every promotion since the baseline; the method is recorded in
[m3-validation.md](m3-validation.md).

## Mapped rows

| Feature | Evidence |
| --- | --- |
| Added Safe Mode | tag 482 (`0x1E2`, `HACK_SAFE_MODE`) read by `FUN_18019B5F0`; the gate vetoes `GJGameLevel::savePercentage` (`FUN_180199DF0`) and marks `PlayLayer::levelComplete` (`FUN_180199E30`). See [safe-mode-feature.md](safe-mode-feature.md). |
| Added Auto Safe Mode | tag 481 (`0x1E1`, `HACK_AUTO_SAFE_MODE`) plus `DAT_18080CBD8`, the `hasCheatedInAttempt` cache refreshed by `FUN_18019B330` after `resetLevel` (`FUN_180199E80`). See [safe-mode-feature.md](safe-mode-feature.md). |
| Added Disable Progress to Noclip | tag 73 (`0x49`, `CHECKBOX_NOCLIP_DISABLE_PROGRESS`) enters the same gate when `FUN_180199520() < 1.0`; the progress accumulators are in `FUN_1801992B0`. See [noclip-feature.md](noclip-feature.md). |
| Added Show Hitboxes | tag 192 (`0xC0`, `HACK_SHOW_HITBOXES`) master toggle; gate `FUN_180203EE0`, draw `FUN_180203F60`→`FUN_180204000`/`FUN_180204150`; per-object draw `FUN_180203070`. Tag 0xC1 (193), 0xD9 (217), 0xDA (218) are runtime-assigned Map A. See [show-hitboxes-feature.md](show-hitboxes-feature.md). |
| Added Show Hitboxes on Death | tag 216 (`0xD8`, `HACK_SHOW_HITBOXES_ON_DEATH`); gate `FUN_180203EE0` branch 2, death-object draw via `FUN_180204000` using `DAT_18080CB80` from the `destroyPlayer` callback. See [show-hitboxes-feature.md](show-hitboxes-feature.md). |
| Added Show Hitbox Trail (Requires Show Hitboxes) | tag 224 (`0xE0`, `HACK_SHOW_HITBOXES_TRAIL`) master toggle; gate `FUN_1801809A0`, trail draw via `FUN_180202BD0` over global point buffer; opacity tag 0xE4 (228). See [show-hitboxes-feature.md](show-hitboxes-feature.md). |
| Added Frame Stepper | tag 302 (`0x12E`, `HACK_FRAME_STEPPER`); registrar `FUN_180173FD0` toggles two UILayer hooks (`onPause` RVA 0x4CDF50, `handleKeypress` RVA 0x4CD830). Tag 357 (0x165, runtime-assigned Map B) is an alternate enable. See [gameplay-hooks.md](gameplay-hooks.md). |
| Added StartPos Switcher | tag 130 (`0x82`, `HACK_STARTPOS_SWITCHER`); gate `FUN_180185710` controls next/prev switching via `FUN_1801857F0`/`FUN_180185910`. Tags 0xA6/0xA7 (166/167) are runtime-assigned Map A. See startpos-switcher-feature.md. |
| Added Solid Wave Trail | tag 396 (`0x18C`, `HACK_SOLID_WAVE_TRAIL`); blend-func toggle in `FUN_1801EECA0` (solid 0x30300000001 vs blended 0x100000302). See cosmetic-feature.md. |
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
| Added No Pulse | tag 413 (`HACK_NO_PULSE`, 0x19D); registered in `FUN_1800afae0` at `0x1800b1384`, callback `FUN_1801713e0` (hooks `PlayLayer::updateVisibility`, RVA 0x3AF3A0). See no-pulse-feature.md. |
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
| Added No (C) Mark | tag 44 (`HACK_NO_C_MARK`, 0x02C); registered in `FUN_1800afae0` at `0x1800b049a`, callback `FUN_180162d20` (hooks `ShareLevelLayer::onShare`, RVA 0x490270). See no-c-mark-feature.md. |
| Added No Respawn Flash | tag 398 (`HACK_NO_RESPAWN_FLASH`, 0x18E); registered in `FUN_1800afae0` at `0x1800b13fb`, callback `FUN_1801714c0` (hooks `PlayerObject::playSpawnEffect`, RVA 0x397480). See no-respawn-flash-feature.md. |
| Added No Death Effect mod | tag 399 (`HACK_NO_DEATH_EFFECT`, 0x18F); registered in `FUN_1800afae0` at `0x1800b0df0`, callback `FUN_180169330` (hooks `PlayerObject::playDeathEffect`, RVA 0x37EF10, with a bare-`ret` detour at `0x180169400`). See no-death-effect-feature.md. |

## Toggles outside the 389

The row universe above is the `Added …` changelog bullets. Three toggles are named elsewhere
in the changelog (under `New For 2.2 Mods`) and are mapped in their own notes, but have no
row here:

| Feature | Evidence |
| --- | --- |
| No Shaders | tag 389 (`HACK_NO_SHADERS`, 0x185); registered in `FUN_1800afae0` at `0x1800b14e9`, callback `FUN_180171620` (hooks `ShaderLayer::performCalculations`, RVA 0x4880B0, and `ShaderLayer::visit`, 0x488A60). See [no-shaders-feature.md](no-shaders-feature.md). |
| No Camera | tag 387 (`HACK_NO_CAMERA`, 0x183); registered in `FUN_1800afae0` at `0x1800b0c14`, callback `FUN_1801680e0` (hooks `CameraTriggerGameObject::triggerObject`, RVA 0x4BD820). See [no-camera-feature.md](no-camera-feature.md). |
| No Camera Zoom | tag 388 (`HACK_NO_CAMERA_ZOOM`, 0x184); registered in `FUN_1800afae0` at `0x1800b0c8b`, callback `FUN_1801682c0` (hooks `EffectGameObject::triggerObject`, RVA 0x4A5F30). See [no-camera-zoom-feature.md](no-camera-zoom-feature.md). |

Adding them to the counts would mix two different universes. If the M3 criterion is later read
as "every feature named anywhere in the changelog", these three are the first rows to add.

| Version | Feature (changelog wording) | Status | Notes |
| --- | --- | --- | --- |
| v9.1.0 | Added Icon Stealer | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.1.0 | Added Selected Objects Only to Show Hitboxes | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v9.1.0 | Added StartPos Switcher Quick Switch toggle | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v9.1.0-beta.6 | Added StartPos Switcher quick switch | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
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
| v9.0.8 | Added Show Layout colour options | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.8 | Added Show Trajectory iterations/thickness options | cluster | [show-trajectory-feature.md](show-trajectory-feature.md) |
| v9.0.8 | Added a small delay when auto-selecting Search | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.8 | Added Festive theme | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.7 | Added No Circle Effect | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.4 | Added Best Run options "Only from Current %" and "Show Prefix" | cluster | [best-run-feature.md](best-run-feature.md) |
| v9.0.4 | Added Multiple Editor Trails | cluster | [creator-feature.md](creator-feature.md) |
| v9.0.4 | Added No Dash Fire | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.4 | Added No New Best Popup | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.4 | Added No Spider Dash | cluster | [render-hooks.md](render-hooks.md) |
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
| v9.0.1 | Added Smart StartPos toggles (Gamemode, Speed, Size, Mirror) | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Iconic (built-in Icon Effects to the Garage) | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added Accurate Percentage | mapped | [best-run-feature.md](best-run-feature.md) | tag 429 (0x1AD, runtime-assigned Map B); `FUN_180165110` hooks `PlayLayer::updateProgressbar` (RVA 0x3B23C0) and formats percentage with configurable decimals (tag 0x1AE = 430). See best-run-feature.md. |
| v9.0.0 | Added Advanced Formatting to Statuses | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.0 | Added Allow Pause Buffering | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Auto Kill | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Ball Rotation Bug | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Classic Pulse (pulsing above 60hz) | cluster | [render-hooks.md](render-hooks.md) |
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
| v9.0.0 | Added No Orb Ring | cluster | [render-hooks.md](render-hooks.md) |
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
| v8.0.2-alpha.1 | Added Auto Music Sync | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v8.0.2-alpha.1 | Added Fast Chests | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.2-alpha.1 | Added No Portal Lightning | mapped | [render-hooks.md](render-hooks.md) | tag 416 (`HACK_NO_PORTAL_LIGHTNING`, 0x1A0); registered in `FUN_1800afae0` at `0x1800b1296`, callback `FUN_180171170` (hooks `GJBaseGameLayer::lightningFlash`). See no-portal-lightning-feature.md. |
| v8.0.2-alpha.1 | Added Main Level Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.2-alpha.1 | Added Disable Keybinds in Editor | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.2-alpha.1 | Added Best Run option to only show current startpos runs | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.1-alpha.9 | Added Stop Triggers on Death | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.1-alpha.9 | Added Auto Pickup Coins | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.1-alpha.9 | Added Auto Song Downloader | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.1-alpha.8 | Added Accurate Save | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.1-alpha.7 | Added Replay with brand new interface | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.1-alpha.7 | Added Replay auto-save | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.1-alpha.7 | Added Safe Mode Popup | cluster | [safe-mode-feature.md](safe-mode-feature.md) |
| v8.0.1-alpha.5 | Added Replay (beta) | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.1-alpha.5 | Added Capture | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.1-alpha.5 | Added Attempt Status | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.1-alpha.5 | Added Menu Keybind Fallthrough option | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.1-alpha.5 | Added menu-wide pasting | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.1-alpha.5 | Added an improved Practice Bug Fix | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.1-alpha.4 | Added No Wave Trail Behind | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 414 (`HACK_NO_WAVE_TRAIL_BEHIND`, 0x19E); registered in `FUN_1800afae0` at `0x1800b182a`, callback `FUN_180172280` (hooks `PlayerObject::activateStreak`). See no-wave-trail-behind-feature.md. |
| v8.0.1-alpha.3 | Added Icon Randomiser with editor support | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.3 | Added Icon Effects with editor support | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.3 | Added Same Dual Colour | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.2 | Added Pitch Shifter | mapped | [audio-sync-feature.md](audio-sync-feature.md) | tag 459 (`HACK_PITCH_SHIFTER`); listener `FUN_18018A740` (tag 0x1CC = 460) calls `FUN_18018A800` → `FUN_18020DA00` (applies pitch to FMOD channels 0–4). See audio-sync-feature.md. |
| v8.0.0-alpha13b | Added Main Menu Gameplay | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha13b | Added TPS Bypass | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 490 (`HACK` TPS Bypass); hooks `GJBaseGameLayer::getModifiedDelta` (RVA 0x2377B0) via `FUN_180196CA0` (registered by `FUN_180196C30`). Tags 0x1E6 (486, TPS value) and 0x1E7 (487, sub-toggle) are runtime-assigned. See gameplay-hooks.md. |
| v8.0.0-alpha13b | Added Hitbox Trail | mapped | [show-hitboxes-feature.md](show-hitboxes-feature.md) | tag 360 (0x168, runtime-assigned Map B) controls trail overlay draw node 0x1E6813C visibility via `FUN_1801589F0`. Main trail draw is `FUN_1801809A0` (tag 224). See show-hitboxes-feature.md. |
| v8.0.0-alpha13b | Added No Pulse | mapped | [render-hooks.md](render-hooks.md) | tag 413 (`HACK_NO_PULSE`, 0x19D); registered in `FUN_1800afae0` at `0x1800b1384`, callback `FUN_1801713e0` (hooks `PlayLayer::updateVisibility`). See no-pulse-feature.md. |
| v8.0.0-alpha13b | Added Real Time to Lock Delta | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha12 | Added Replay | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.0-alpha12 | Added Replay for Platformer mode | cluster | [replay-feature.md](replay-feature.md) |
| v8.0.0-alpha12 | Added Tooltips | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha12 | Added Demonlist, Challengelist, Speedrun & Leaderboard Rulesets | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha12 | Added Instant Complete | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha12 | Added Classic Borderless Fullscreen | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha12 | Added Physics Sync | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha12 | Added Confirm Reset | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha12 | Added Confirm Full-Reset | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha12 | Added Confirm Practice | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha12 | Added Hide Player | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha12 | Added Hide Pause Button | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha12 | Added Thread Priority | mapped | [render-hooks.md](render-hooks.md) | tag 454 (`HACK_THREAD_PRIORITY`); `FUN_18018ADF0`/`FUN_18018AE50` call `SetThreadPriority` when enabled. Tag 0x1C7 (455) is the priority level. See render-hooks.md. |
| v8.0.0-alpha12 | Added Switch After Death to StartPos Switcher | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.0-alpha12 | Added support for NodeIDs (Geode) | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha10b | Added Wave Trail Fix | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha10b | Added Smooth Editor Trail | cluster | [creator-feature.md](creator-feature.md) |
| v8.0.0-alpha10b | Added Screenshots | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha10b | Added Practice Bug Fix | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha10b | Added custom click/move keybinds | cluster | [input-hooks.md](input-hooks.md) |
| v8.0.0-alpha10b | Added Best Run | cluster | [best-run-feature.md](best-run-feature.md) |
| v8.0.0-alpha10b | Added Collect Coins in Practice | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha10b | Added Checkpoint Limit Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha10b | Added No "Do Not" Flip | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 412 (`HACK_NO_DO_NOT_FLIP`, 0x19C); registered in `FUN_1800afae0` at `0x1800b0e67`, callback `FUN_1801696e0` (hooks `EndLevelLayer::customSetup` + `PlayLayer::setupHasCompleted`). See no-do-not-flip-feature.md. |
| v8.0.0-alpha10b | Added Meta Status | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha10b | Added Toolbox Button Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha10b | Added Place Over | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha9b | Added auto update to Geode builds | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Auto Deafen | mapped | [audio-sync-feature.md](audio-sync-feature.md) | tag 279 (`HACK_AUTO_DEAFEN`, 0x117); registered in `FUN_1800afae0` at `0x1800b1dc8`, callback `FUN_180174a80` (hooks `PlayLayer::resetLevel`). See auto-deafen-feature.md. |
| v8.0.0-alpha9 | Added Show Triggers | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha9 | Added Jumpscare | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Replay Last Checkpoint | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added 0% Practice Complete | mapped | [gameplay-hooks.md](gameplay-hooks.md) | tag 296 (`HACK_0_PRACTICE_COMPLETE`, static map entry #0); read via `hackpro::Config` vtable (`FUN_1800917B0` at 0x180544FF0). No gameplay code reads it directly; accessed through exported `isHackEnabled` API. See gameplay-hooks.md. |
| v8.0.0-alpha9 | Added Uncomplete Level | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Trigger Value Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Hide Pause Menu | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha9 | Added Hide UI | cluster | [render-hooks.md](render-hooks.md) |
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
| v8.0.0-alpha6 | Added Show Layout | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha6 | Added Endscreen Hider | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha5 | Added Smart StartPos | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added Respawn Time modifier | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added Confirm Exit | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha5 | Added Allow Low Volume | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added No Particles | mapped | [render-hooks.md](render-hooks.md) | tag 402 (`HACK_NO_PARTICLES`, 0x192); registered in `FUN_1800afae0` at `0x1800b11a8`, callback `FUN_1801703b0`. See no-particles-feature.md. |
| v8.0.0-alpha5 | Added No Wave Trail | mapped | [render-hooks.md](render-hooks.md) | tag 397 (`HACK_NO_WAVE_TRAIL`, 0x18D); registered in `FUN_1800afae0` at `0x1800b17b3`, callback `FUN_1801721f0` (hooks `HardStreak::updateStroke`). See no-wave-trail-feature.md. |
| v8.0.0-alpha5 | Added Solid Wave Trail | mapped | [cosmetic-feature.md](cosmetic-feature.md) | tag 396 (`HACK_SOLID_WAVE_TRAIL`); blend-func toggle in `FUN_1801EECA0` (solid 0x30300000001 vs blended 0x100000302). See cosmetic-feature.md. |
| v8.0.0-alpha5 | Added No Respawn Flash | mapped | [render-hooks.md](render-hooks.md) | tag 398 (`HACK_NO_RESPAWN_FLASH`, 0x18E); registered in `FUN_1800afae0` at `0x1800b13fb`, callback `FUN_1801714c0` (hooks `PlayerObject::playSpawnEffect`). See no-respawn-flash-feature.md. |
| v8.0.0-alpha5 | Added Force StartPos Reset Camera | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added StartPos Switcher keybind options | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.0-alpha5 | Added StartPos Switcher interface | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.0-alpha5 | Added Hide Interface to StartPos Switcher | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.0-alpha5 | Added Inject DLL | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha4 | Added Safe Mode | mapped | [safe-mode-feature.md](safe-mode-feature.md) |
| v8.0.0-alpha4 | Added Auto Safe Mode | mapped | [safe-mode-feature.md](safe-mode-feature.md) |
| v8.0.0-alpha4 | Added Platformer Practice | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha4 | Added Trail Cutting | mapped | [render-hooks.md](render-hooks.md) | tag 392 (`HACK_TRAIL_CUTTING`, 0x188); registered in `FUN_1800afae0` at `0x1800b1be2`, callback `FUN_1801732c0` (hooks `cocos2d::CCMotionStreak::initWithFade`). See trail-cutting-feature.md. |
| v8.0.0-alpha4 | Added Zero Delay | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha4 | Added Text Length Bypass | cluster | [ui-addons.md](ui-addons.md) |
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
| v7.0 (Full) | Added Replay Last Checkpoint to Endscreen | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
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
| v6.2.0 (Full) | Added Replay Last Checkpoint mod | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
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
