# Best Run feature cluster

- **Question** — which code and settings implement Mega Hack's Best Run features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

| Setting key | Observed value | Role |
| --- | --- | --- |
| `STATUS/BEST_RUN/DROPDOWN/ACCURATE_PERCENTAGE` | False | Enables accurate percentage in Best Run |
| `STATUS/BEST_RUN/DROPDOWN/ONLY_CURRENT` | True | Only show current StartPos runs |
| `STATUS/BEST_RUN/DROPDOWN/PREFIX` | True | Reused row for native Import .dt |
| `COSMETIC/HACK_NO_NEW_BEST_POPUP` | (listed) | Suppresses the New Best popup |

## Evidence — New Best label

From [new-best-label.md](new-best-label.md):

- `PlayLayer::showNewBest` is hooked at `0x180169DE0` by `FUN_180169EB0`
  (`0x180169EB0`), with `PlayLayer::destroyPlayer` hooked at `0x180169F20`.
- Setting `245` (`0xF5`) suppresses the whole path.
- Language key `0052` selects opacity `0.5`; otherwise opacity `1.0`.
- Settings `247` (`0xF7`), `501` (`0x1F5`) and `502` (`0x1F6`) gate the
  opacity scale inside `FUN_18017F000`.

## Evidence — native storage

From [persistence.md](persistence.md) and [save-data-hooks.md](save-data-hooks.md):

- Mega Hack stores no Best Run data itself.
- Its only storage-side hook is `GJGameLevel::savePercentage`
  (`0x180199D10` / callback `0x180199DF0`), which can veto the call but never
  write.
- Best Run is GD's own saved percentage.

## Evidence — native Import .dt

From [import-dt.md](import-dt.md):

- The native action reuses the former Show Prefix row.
- Its tag is resolved after core load and intercepted by the boolean setter.
- The file picker is `FUN_1800C6300` wrapping `GetOpenFileNameW`.
- Cancelling the picker makes no changes.

## Open questions

- UNVERIFIED: the exact GD-side fields carrying
  `(level, startPercent) -> bestEndPercent`. They are not recoverable from
  Mega Hack's binary.
- UNVERIFIED: which settings keys correspond to tags 247, 501 and 502.
  245 is the Map B hack-flag tag `HACK_RESPAWN_TIME` (static label map plus the
  `isHackEnabled` getter path); its runtime effect on the label is still to confirm. See
  [static-hack-label-map.md](static-hack-label-map.md).
- UNVERIFIED: whether `STATUS/BEST_RUN/DROPDOWN/PREFIX` is the reused row's
  persisted display state or a separate setting.
