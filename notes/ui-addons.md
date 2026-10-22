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
