# Render hooks cluster

- **Question** — which rendering paths does Mega Hack hook, and what do they share?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Filter `recovered/functions/_hook_sites.tsv` for GD methods whose names contain
   `draw`, `Render`, `Shader`, `Camera`, `Visibility`, `Ghost`, `Streak`, `CircleWave`,
   `swapBuffers`, or `drawScene`.
2. Cross-check registrar and callback VAs against `notes/hook-dispatch.md`.

## Evidence — render hook sites

| Registrar VA | GD method | Target | Callback VA | Callback kind | Helper |
| --- | --- | --- | --- | --- | --- |
| `0x18014ED70` | `cocos2d::CCEGLView::swapBuffers` | name-resolved | `0x18014F9D0` | function | `0x18014FEE0` |
| `0x180156D00` | `cocos2d::CCDirector::drawScene` | name-resolved | `0x180156ED0` | function | `0x180157D50` |
| `0x180170DC0` | `cocos2d::CCParticleSystemQuad::draw` | name-resolved | `0x180170FB0` | data | `0x180170FC0` |
| `0x180171DB0` | `cocos2d::CCMotionStreak::draw` | name-resolved | `0x180171FA0` | data | `0x180171FB0` |
| `0x180171620` | `ShaderLayer::performCalculations` | RVA `0x4880B0` | `0x1801716F0` | data | `0x180171710` |
| `0x180171620` | `ShaderLayer::visit` | RVA `0x488A60` | `0x180171700` | data | `0x180171710` |
| `0x180171170` | `GJBaseGameLayer::lightningFlash` | RVA `0x246D00` | `0x1801711E0` | data | `0x1801711F0` |
| `0x1801713E0` | `PlayLayer::updateVisibility` | RVA `0x3AF3A0` | `0x180171450` | function | `0x1800AB9F0` |
| `0x1801582C0` | `PlayLayer::updateVisibility` | RVA `0x3AF3A0` | `0x180158740` | function | `0x1800AB9F0` |
| `0x1801582C0` | `LevelEditorLayer::updateVisibility` | RVA `0x2D8E70` | `0x180158830` | function | `0x180158CC0` |
| `0x180197020` | `GJBaseGameLayer::updateCamera` | RVA `0x23BCF0` | `0x180197440` | label | `0x180172EC0` |
| `0x18019ACB0` | `CCCircleWave::draw` | RVA `0x42E40` | `0x18019AD00` | function | `0x18019AEA0` |
| `0x1800A83B0` | `PlayerObject::toggleGhostEffect` | RVA `0x39EE30` | `0x18010BD80` | function | `0x1800AB3D0` |
| `0x180169AE0` | `PlayerObject::toggleGhostEffect` | RVA `0x39EE30` | `0x180169B50` | data | `0x1800AB3D0` |
| `0x180167A60` | `PlayerObject::toggleVisibility` | RVA `0x38B440` | `0x180167B10` | label | `0x180167B20` |
| `0x180172280` | `PlayerObject::activateStreak` | RVA `0x3A0090` | `0x1801722F0` | function | `0x1800AB230` |

## Pattern

- The global render loop is hooked at two choke points:
  - `cocos2d::CCDirector::drawScene` (`0x180156D00`);
  - `cocos2d::CCEGLView::swapBuffers` (`0x18014ED70`).
- Particle and streak rendering are intercepted as data-slot callbacks rather than
  normal functions (`CCParticleSystemQuad::draw`, `CCMotionStreak::draw`), which means
  their callback bodies are not split into standalone recovered functions.
- Visibility is hooked repeatedly across ownership contexts:
  - `PlayLayer::updateVisibility` twice (`0x1801582C0`, `0x1801713E0`);
  - `LevelEditorLayer::updateVisibility` once (`0x1801582C0`);
  - `PlayerObject::toggleVisibility` once (`0x180167A60`).
- Ghost and streak effects are hooked from two different registrars each
  (`PlayerObject::toggleGhostEffect` at `0x1800A83B0` and `0x180169AE0`;
  `PlayerObject::activateStreak` at `0x180172280`), suggesting the same visual feature
  is observed from more than one call site.
- `ShaderLayer` is hooked as a pair: `performCalculations` and `visit` share helper
  `FUN_180171710`, so its calculations and its draw are treated as one unit.

## Open questions

- UNVERIFIED: which settings keys gate each visual effect. The render callbacks shown
  here do not expose their tags in the recovered snippets.
- UNVERIFIED: the bodies of the data-slot callbacks (`0x180170FB0`,
  `0x180171FA0`, `0x1801716F0`, `0x180171700`, `0x180169B50`) — they are not split
  into standalone functions, so they must be read from the registrar.
- UNVERIFIED: whether `CCCircleWave::draw` and `GJBaseGameLayer::lightningFlash`
  are feature hooks or incidental registrations.
- UNVERIFIED: whether any of these hooks can veto rendering, or only observe it.
