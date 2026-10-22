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

All 389 rows below were `cluster` at the 2026-10-23 baseline. Three now have per-feature
evidence and are `mapped`: **Added Safe Mode**, **Added Auto Safe Mode** and **Added
Disable Progress to Noclip** (see below). The other 386 remain `cluster`. A keyword check
at the baseline found that **185 of 389** rows point at a note that never mentions the
feature at all, so for those even the cluster assignment is only a lead. That count
predates the three promotions; the method is recorded in
[m3-validation.md](m3-validation.md).

## Mapped rows

| Feature | Evidence |
| --- | --- |
| Added Safe Mode | tag 482 (`0x1E2`, `HACK_SAFE_MODE`) read by `FUN_18019B5F0`; the gate vetoes `GJGameLevel::savePercentage` (`FUN_180199DF0`) and marks `PlayLayer::levelComplete` (`FUN_180199E30`). See [safe-mode-feature.md](safe-mode-feature.md). |
| Added Auto Safe Mode | tag 481 (`0x1E1`, `HACK_AUTO_SAFE_MODE`) plus `DAT_18080CBD8`, the `hasCheatedInAttempt` cache refreshed by `FUN_18019B330` after `resetLevel` (`FUN_180199E80`). See [safe-mode-feature.md](safe-mode-feature.md). |
| Added Disable Progress to Noclip | tag 73 (`0x49`, `CHECKBOX_NOCLIP_DISABLE_PROGRESS`) enters the same gate when `FUN_180199520() < 1.0`; the progress accumulators are in `FUN_1801992B0`. See [noclip-feature.md](noclip-feature.md). |

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
| v9.0.11 | Added "Requires Show Hitboxes" option to Show Hitbox Trail | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.10 | Added Icon Randomiser | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.10 | Added positions top-centre and bottom-centre Status label positions | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.10 | Added Vanilla Progress Bar to Iconic | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.9 | Added Geode DevTools shortcut | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.9 | Added Player 1 on Top | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.8 | Added Auto Clicker | cluster | [input-hooks.md](input-hooks.md) |
| v9.0.8 | Added Auto-Select toggle for Search | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.8 | Added Coin Shower | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.8 | Added No End Shake | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.8 | Added No Robot/Swing Fire | cluster | [render-hooks.md](render-hooks.md) |
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
| v9.0.1 | Added Interface tab to disable Mega Hack UI changes | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.1 | Added Smart StartPos toggles (Gamemode, Speed, Size, Mirror) | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Iconic (built-in Icon Effects to the Garage) | cluster | [iconic-feature.md](iconic-feature.md) |
| v9.0.0 | Added Accurate Percentage | cluster | [best-run-feature.md](best-run-feature.md) |
| v9.0.0 | Added Advanced Formatting to Statuses | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.0 | Added Allow Pause Buffering | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Auto Kill | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Ball Rotation Bug | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Classic Pulse (pulsing above 60hz) | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Classic Wave Trail (reimplementation of the 2.1 wave) | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Click Between Frames toggle | cluster | [input-hooks.md](input-hooks.md) |
| v9.0.0 | Added Compact Lists | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.0 | Added Confirm Exit (shortcut for the in-game setting) | cluster | [ui-addons.md](ui-addons.md) |
| v9.0.0 | Added Custom Background (successor to Transparent BG) | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
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
| v9.0.0 | Added Menu Snow in December | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v9.0.0 | Added Player on Top | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Pause During Complete | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v9.0.0 | Added Shipcopter | cluster | [render-hooks.md](render-hooks.md) |
| v9.0.0 | Added Show Total Attempts | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v9.0.0 | Added Show Trajectory | cluster | [show-trajectory-feature.md](show-trajectory-feature.md) |
| v9.0.0 | Added Transition Customiser (with duration) | cluster | [ui-addons.md](ui-addons.md) |
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
| v9.0.0 | Added multi-lingual support | cluster | [ui-addons.md](ui-addons.md) |
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
| v8.0.2-alpha.1 | Added No Portal Lightning | cluster | [render-hooks.md](render-hooks.md) |
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
| v8.0.1-alpha.4 | Added No Wave Trail Behind | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.1-alpha.3 | Added Icon Randomiser with editor support | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.3 | Added Icon Effects with editor support | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.3 | Added Same Dual Colour | cluster | [iconic-feature.md](iconic-feature.md) |
| v8.0.1-alpha.2 | Added Pitch Shifter | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v8.0.0-alpha13b | Added Main Menu Gameplay | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha13b | Added TPS Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha13b | Added Hitbox Trail | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v8.0.0-alpha13b | Added No Pulse | cluster | [render-hooks.md](render-hooks.md) |
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
| v8.0.0-alpha12 | Added Thread Priority | cluster | [render-hooks.md](render-hooks.md) |
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
| v8.0.0-alpha10b | Added No "Do Not" Flip | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha10b | Added Meta Status | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha10b | Added Toolbox Button Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha10b | Added Place Over | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha9b | Added auto update to Geode builds | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Auto Deafen | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v8.0.0-alpha9 | Added Show Triggers | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha9 | Added Jumpscare | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Replay Last Checkpoint | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added 0% Practice Complete | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Uncomplete Level | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Trigger Value Bypass | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha9 | Added Hide Pause Menu | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha9 | Added Hide UI | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha9 | Added Restart, Practice & folder shortcuts | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha8 | Added Show Hitboxes | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v8.0.0-alpha8 | Added Show Hitboxes on Death | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v8.0.0-alpha8 | Added Wave Pulse Size | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha8 | Added No (C) Mark | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha8 | Added No Shake | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha8 | Added Clock | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha7 | Added Classic Particles | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha7 | Added Noclip Accuracy | cluster | [noclip-feature.md](noclip-feature.md) |
| v8.0.0-alpha7 | Added Noclip Limits | cluster | [noclip-feature.md](noclip-feature.md) |
| v8.0.0-alpha7 | Added Cheat Indicator with breakdown | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha7 | Added FPS & CPS Statuses | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha7 | Added Message, Percentage & Level Time Statuses | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v8.0.0-alpha6 | Added Show Layout | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha6 | Added Endscreen Hider | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha5 | Added Smart StartPos | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added Respawn Time modifier | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added Confirm Exit | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha5 | Added Allow Low Volume | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added No Particles | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha5 | Added No Wave Trail | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha5 | Added Solid Wave Trail | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha5 | Added No Respawn Flash | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha5 | Added Force StartPos Reset Camera | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha5 | Added StartPos Switcher keybind options | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.0-alpha5 | Added StartPos Switcher interface | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.0-alpha5 | Added Hide Interface to StartPos Switcher | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
| v8.0.0-alpha5 | Added Inject DLL | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha4 | Added Safe Mode | mapped | [safe-mode-feature.md](safe-mode-feature.md) |
| v8.0.0-alpha4 | Added Auto Safe Mode | mapped | [safe-mode-feature.md](safe-mode-feature.md) |
| v8.0.0-alpha4 | Added Platformer Practice | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha4 | Added Trail Cutting | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha4 | Added Zero Delay | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v8.0.0-alpha4 | Added Text Length Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha4 | Added Slider Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v8.0.0-alpha4 | Added No Trail | cluster | [render-hooks.md](render-hooks.md) |
| v8.0.0-alpha4 | Added No Wave Pulse | cluster | [render-hooks.md](render-hooks.md) |
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
| v7.1 (Full) | Added Frame Stepper | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
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
| v7.1 (Full) | Added No Robot Fire | cluster | [render-hooks.md](render-hooks.md) |
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
| v7.0 (Full) | Added StartPos Switcher | cluster | [startpos-switcher-feature.md](startpos-switcher-feature.md) |
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
| v6.3.0 (Full) | Added Corrective Music Sync | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v6.3.0 (Full) | Added Glow Customisation | cluster | [cosmetic-feature.md](cosmetic-feature.md) |
| v6.3.0 (Full) | Added Hitbox Viewer | cluster | [show-hitboxes-feature.md](show-hitboxes-feature.md) |
| v6.3.0 (Full) | Added Legacy Mode to Speedhack | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.3.0 (Full) | Added Level Uncompleter | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.3.0 (Full) | Added Music-synced Icons | cluster | [audio-sync-feature.md](audio-sync-feature.md) |
| v6.3.0 (Full) | Added NoClip Accuracy screen tint | cluster | [noclip-feature.md](noclip-feature.md) |
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
| v6.2.0 (Full) | Added Borderless Fullscreen | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added Coins Show as Uncollected mod | cluster | [gameplay-hooks.md](gameplay-hooks.md) |
| v6.2.0 (Full) | Added Col1, Col2, & Wave Trail to RGB Icons | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added ColourID Bypass | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Copy LevelID Fix | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Demon List integration | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added FPS Counter | cluster | [status-labels-feature.md](status-labels-feature.md) |
| v6.2.0 (Full) | Added Level Restart button | cluster | [ui-addons.md](ui-addons.md) |
| v6.2.0 (Full) | Added Lock Cursor | cluster | [render-hooks.md](render-hooks.md) |
| v6.2.0 (Full) | Added Mini Cube Icon mod | cluster | [iconic-feature.md](iconic-feature.md) |
| v6.2.0 (Full) | Added No Death Effect mod | cluster | [render-hooks.md](render-hooks.md) |
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
