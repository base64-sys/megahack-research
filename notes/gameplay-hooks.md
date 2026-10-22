# Gameplay hooks cluster

- **Question** — which gameplay paths does Mega Hack hook, and what shared patterns do
  they use?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Filter `recovered/functions/_hook_sites.tsv` for GD methods in `PlayerObject`,
   `GJBaseGameLayer`, and the collision / button / update families.
2. Read the shared helpers named by those rows.
3. Cross-check against `notes/hook-dispatch.md` and `notes/host-methods.md`.

## Evidence — core gameplay hook sites

| Registrar VA | GD method | Target | Callback VA | Callback kind | Helper |
| --- | --- | --- | --- | --- | --- |
| `0x180172160` | `PlayerObject::update` | RVA `0x388D80` | `0x1801721D0` | label | `0x1800AAC20` |
| `0x180173920` | `PlayerObject::update` | RVA `0x388D80` | `0x180173990` | function | `0x1800AAC20` |
| `0x1801990B0` | `PlayerObject::collidedWithObjectInternal` | RVA `0x391A70` | `0x180199440` | function | `0x180199730` |
| `0x1801990B0` | `PlayerObject::collidedWithSlopeInternal` | RVA `0x38F810` | `0x1801994B0` | function | `0x180199A50` |
| `0x180165890` | `PlayerObject::ringJump` | RVA `0x398C00` | `0x180165B60` | function | `0x180165DC0` |
| `0x180165890` | `PlayerObject::hitGround` | RVA `0x39BF30` | `0x180165BC0` | function | `0x180166010` |
| `0x180173D00` | `PlayerObject::playerDestroyed` | RVA `0x397D40` | `0x180173E30` | function | `0x180167B20` |
| `0x180172DE0` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180172E50` | function | `0x180172EC0` |
| `0x180177B00` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180177D90` | function | `0x180172EC0` |
| `0x180197020` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180197270` | function | `0x180172EC0` |
| `0x180198BF0` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180198CD0` | function | `0x180172EC0` |
| `0x18019CA80` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x18019E560` | label | `0x180172EC0` |
| `0x180167210` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180167280` | function | `0x180167330` |
| `0x180174590` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180174600` | function | `0x180167330` |
| `0x180176EA0` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180176F10` | function | `0x180167330` |
| `0x18017A710` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x18017A780` | function | `0x180167330` |
| `0x180181540` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180181AA0` | function | `0x180167330` |
| `0x1801746F0` | `GJBaseGameLayer::processCommands` | RVA `0x239C60` | `0x180174760` | function | `0x180174920` |
| `0x1801801D0` | `GJBaseGameLayer::processCommands` | RVA `0x239C60` | `0x180180240` | function | `0x180174920` |
| `0x18019CA80` | `GJBaseGameLayer::processCommands` | RVA `0x239C60` | `0x18019E490` | function | `0x180174920` |
| `0x180197020` | `GJBaseGameLayer::updateCamera` | RVA `0x23BCF0` | `0x180197440` | label | `0x180172EC0` |
| `0x18015C3F0` | `GJBaseGameLayer::shouldExitHackedLevel` | RVA `0x20B050` | `0x18015C460` | label | `0x18015B120` |
| `0x180198BF0` | `GJBaseGameLayer::handleButton` | RVA `0x2338E0` | `0x180198D00` | function | `0x180198EF0` |
| `0x18019CA80` | `GJBaseGameLayer::handleButton` | RVA `0x2338E0` | `0x18019E4C0` | function | `0x180198EF0` |

## Pattern

- `GJBaseGameLayer::update` is the central gameplay choke point: it is registered
  five times (`0x180172DE0`, `0x180177B00`, `0x180178C60`, `0x180197020`,
  `0x180198BF0`, `0x18019CA80`) and all converge on helper `FUN_180172EC0`.
- `GJBaseGameLayer::collisionCheckObjects` is registered five times
  (`0x180167210`, `0x180174590`, `0x180176EA0`, `0x18017A710`,
  `0x180181540`) and all converge on helper `FUN_180167330`.
- `GJBaseGameLayer::processCommands` is registered three times
  (`0x1801746F0`, `0x1801801D0`, `0x18019CA80`) and all converge on helper
  `FUN_180174920`.
- `PlayerObject::update` is registered twice (`0x180172160`, `0x180173920`)
  and both converge on helper `FUN_1800AAC20`.
- `GJBaseGameLayer::handleButton` is registered twice (`0x180198BF0`,
  `0x18019CA80`) and both converge on helper `FUN_180198EF0`.

The repeated registration of the same method from different registrars is the
same pattern seen in the input cluster: Mega Hack observes the same gameplay event
from several ownership or timing contexts, then funnels each family through one
shared helper.

## Open questions

- UNVERIFIED: which settings tags gate each gameplay hook. The recovered callbacks
  shown here do not expose their tags.
- UNVERIFIED: the behaviour of shared helpers `FUN_180172EC0`,
  `FUN_180167330`, `FUN_180174920`, `FUN_1800AAC20` and `FUN_180198EF0`;
  only their call shape and convergence are recorded here.
- UNVERIFIED: whether `shouldExitHackedLevel` is a veto hook or an observer.
- UNVERIFIED: whether the five `collisionCheckObjects` registrations differ by
  object class, layer, or timing.
