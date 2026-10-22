# Save-data hooks cluster

- **Question** — which save, checkpoint and progress paths does Mega Hack hook?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Filter `recovered/functions/_hook_sites.tsv` for GD methods whose names contain
   `save`, `Save`, `checkpoint`, `Checkpoint`, `Best`, `best`, `progress` or
   `Progress`.
2. Cross-check registrar and callback VAs against `notes/hook-dispatch.md` and
   `notes/persistence.md`.

## Evidence — save-data hook sites

| Registrar VA | GD method | Target | Callback VA | Callback kind | Helper |
| --- | --- | --- | --- | --- | --- |
| `0x1800A8020` | `AppDelegate::trySaveGame` | RVA `0x83500` | `0x18010B000` | function | `0x1800A9E00` |
| `0x180151B40` | `AppDelegate::trySaveGame` | RVA `0x83500` | `0x180151B90` | function | `0x1800A9E00` |
| `0x1800A83B0` | `PlayLayer::updateProgressbar` | RVA `0x3B23C0` | `0x18010C0A0` | function | `0x1800AB850` |
| `0x180165040` | `PlayLayer::updateProgressbar` | RVA `0x3B23C0` | `0x180165110` | function | `0x1800AB850` |
| `0x180190E50` | `PlayLayer::toggleProgressbar` | RVA `0x3B2760` | `0x1801913C0` | function | `0x1800AB850` |
| `0x180176250` | `PlayLayer::storeCheckpoint` | RVA `0x3B74A0` | `0x1801762C0` | function | `0x180176300` |
| `0x18017B580` | `PlayLayer::createCheckpoint` | RVA `0x3B4FC0` | `0x18017B5D0` | function | `0x18017DC80` |
| `0x18019CA80` | `PlayerObject::tryPlaceCheckpoint` | RVA `0x3A32D0` | `0x18019E680` | function | `0x1800AB230` |
| `0x180169DE0` | `PlayLayer::showNewBest` | RVA `0x3A9550` | `0x180169EB0` | function | `0x180169FA0` |
| `0x180199D10` | `GJGameLevel::savePercentage` | RVA `0x16C8B0` | `0x180199DF0` | function | `0x18019A0C0` |
| `0x18015E560` | `EditorPauseLayer::saveLevel` | RVA `0xDD840` | `0x18015E5D0` | function | `0x18015E770` |
| `0x180151280` | `EditorPauseLayer::onSaveAndPlay` | RVA `0xDDEE0` | `0x180151440` | function | `0x180151890` |
| `0x180151280` | `EditorPauseLayer::onSaveAndExit` | RVA `0xDE260` | `0x180151470` | function | `0x180151890` |
| `0x180151280` | `EditorPauseLayer::onExitNoSave` | RVA `0xDE370` | `0x1801514A0` | function | `0x180151890` |
| `0x18017F0F0` | `GameObject::saveActiveColors` | RVA `0x1A08D0` | `0x18017FA60` | function | `0x18017EB00` |

## Pattern

- `AppDelegate::trySaveGame` is registered twice (`0x1800A8020`,
  `0x180151B40`) and both converge on helper `FUN_1800A9E00`.
- `PlayLayer::updateProgressbar` is registered twice (`0x1800A83B0`,
  `0x180165040`) and both converge on helper `FUN_1800AB850`.
- Checkpoint handling is split between three methods:
  - `PlayLayer::storeCheckpoint` (`0x180176250`);
  - `PlayLayer::createCheckpoint` (`0x18017B580`);
  - `PlayerObject::tryPlaceCheckpoint` (`0x18019CA80`).
- `GJGameLevel::savePercentage` (`0x180199D10`) is the only storage-side hook and
  its callback (`0x180199DF0`) can veto the original call entirely.
- `PlayLayer::showNewBest` (`0x180169DE0`) is an opacity hook, not a storage hook:
  it computes `1.0` or `0.5` opacity depending on whether language key `0052` is
  present (see [new-best-label.md](new-best-label.md)).
- `GameObject::saveActiveColors` (`0x18017F0F0`) is the only object-state save hook
  in this cluster.

## Relation to persistence

Mega Hack stores no Best Run data of its own. Its save-related role is to veto
`GJGameLevel::savePercentage` when the shared gate `FUN_18019B5F0()` is active, and
otherwise to forward to GD's original. The flush points are GD's, except for the
safe-mode re-evaluation in the `resetLevel` hook. See
[persistence.md](persistence.md) and [save-lifecycle.md](save-lifecycle.md).

## Open questions

- UNVERIFIED: which settings tags gate each save-data hook.
- UNVERIFIED: whether `EditorPauseLayer::saveLevel` and the `onSaveAnd*` hooks are
  feature hooks or incidental registrations.
- UNVERIFIED: whether `storeCheckpoint` / `createCheckpoint` / `tryPlaceCheckpoint`
  are observed, modified, or vetoed.
- UNVERIFIED: whether `AppDelegate::trySaveGame` hooks are feature hooks or
  diagnostics.
