# Save lifecycle and flush points

- **Question** — what does Mega Hack do at `postUpdate`, `resetLevel`,
  `PlayLayer::onQuit` and shutdown, and where are its flush points?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — hook shapes

Hook registration is mechanical: `(resolver, targetFromGDBase(rva), callback, name, -32, 1)`
under a `TulipConvention`; originals are called through `FUN_1801A33C0(FUN_18010DDC0() + rva)`.
See [hook-dispatch.md](hook-dispatch.md).

### `resetLevel` — `FUN_180199E80`, 19 bytes

```c
void FUN_180199e80(void) {
    FUN_1801a4300();   // original PlayLayer::resetLevel
    FUN_18019b330();   // safe-mode cache refresh
}
```

- VA `0x180199E80`, RVA `0x199E80`.
- The follow-up `FUN_18019B330` (`0x18019B330`) only caches
  `mega::utils::safe_mode::areCheatsEnabled()` into `DAT_18080CBD8`.
- There is no file I/O here. "Flush" in the release is the safe-mode re-evaluation,
  not a persistence write.

### `levelComplete` — `FUN_180199E30`, 67 bytes

```c
cVar1 = FUN_18019b5f0();
if (cVar1 != '\0' && *(char *)(param_1 + 0x3230) == '\0') {
    *(char *)(param_1 + 0x3230) = 1;   // re-entrancy flag
    FUN_1801a5190(param_1);            // original
    *(char *)(param_1 + 0x3230) = 0;
    return;
}
FUN_1801a5190(param_1);
```

- VA `0x180199E30`, RVA `0x199E30`.
- `PlayLayer + 0x3230` is a one-byte re-entrancy flag set around the original call.

### `savePercentage` — `FUN_180199DF0`, 64 bytes

```c
cVar1 = FUN_18019b5f0();
if (cVar1 != '\0') return;            // veto
FUN_1801462c0(param_1, param_2, param_3, param_4);   // original
```

- VA `0x180199DF0`, RVA `0x199DF0`.
- This hook can **block** GD from learning that a percentage was saved. It is the only
  storage-side hook and it is a veto, not a writer.

### `postUpdate` — `FUN_1801992B0`, 317 bytes

- VA `0x1801992B0`, RVA `0x1992B0`.
- It calls the original `FUN_1801A37C0`, then, when the three in-level conditions hold,
  accumulates frame time into `DAT_18080CB70` and (when `DAT_18080CB68 == 1`)
  `DAT_18080CB78`; the checkpoint vector at `DAT_18080CB50` gets an entry on the first
  frame of a cheat segment, and its last entry is updated on the first clean frame after
  one.
- At the end of the frame it sets `DAT_18080CB69 = DAT_18080CB68` and then clears
  `DAT_18080CB68`; the delayed copy is `CB69`, not `CB68`. The `= 1` writer is
  `0x1801993FD`, inside the `PlayLayer::destroyPlayer` callback
  ([safe-mode-feature.md](safe-mode-feature.md#the-cheat-frame-setter-playlayerdestroyplayer)).
- No file I/O occurs here. The "postUpdate" flush point in the release is the
  anti-cheat progress accounting, not a disk write.

### `PlayLayer::onQuit` — `FUN_180174DE0`, 20 bytes

```c
void FUN_180174de0(void) {
    FUN_1801a5260();       // original PlayLayer::onQuit
    FUN_180174e00(0);     // conditional body
}
```

- VA `0x180174DE0`, RVA `0x174DE0`.
- The tail body `FUN_180174E00(0)` (`0x180174E00`) reads setting tag `0x11D`
  (`getConfigVar<hackpro::Bytes>`) and, when it differs from the cached
  `DAT_18080C580`, calls `FUN_18019EF30`, which replays the stored byte sequence
  through `keybd_event` (press then release).
- So the onQuit path is a key-replay hook, not a persistence flush.

## Evidence — startup reads

- `FUN_180042D70` (`0x180042D70`) is the sole caller of `FUN_180042FB0`
  (`0x180042FB0`), which reads `%LOCALAPPDATA%\Octavus\{data.json,save.json}`.
  It has two callers, so it is not strictly startup-only; whether one path is a
  migration path is untraced.
- `hackpro.json` is read by `FUN_1800BF190` and written by `FUN_1800BF400`
  (`0x1800BF190` / `0x1800BF400`), both with zero direct callers (virtual dispatch).

## Evidence — shutdown destructor

- `FUN_180001370` (`0x180001370`) is registered through `atexit` by the TLS-guarded
  initialiser at `DAT_1808008D8` (see [entry-path.md](entry-path.md)).
- It resets `CoreDirector` vtables (`DAT_1808004D0`, `DAT_180800840`,
  `DAT_180800848`) to `CoreDirector::vftable`, frees two allocations guarded by
  `DAT_1808008D0` and `DAT_1808008A8`, and calls `FUN_1800020C0(&DAT_1808004D0)`.
- No file write is visible in this destructor.

## Conclusion

1. Mega Hack's lifecycle hooks are small and mostly observational: `resetLevel`
   refreshes safe mode, `levelComplete` guards re-entrancy, `savePercentage` can veto,
   `postUpdate` accounts cheat-attributed progress, and `onQuit` performs key replay.
2. None of these hooks writes Mega Hack's own configuration or a sidecar database.
   Configuration persistence is `hackpro.json` (`FUN_1800BF190` / `FUN_1800BF400`),
   reached through virtual dispatch, and GD's save files are read through
   `FUN_180042FB0`.
3. The shutdown destructor releases CoreDirector state and two allocations; it is not a
   flush point for settings.

## Open questions

- UNVERIFIED: what triggers `FUN_1800BF400` (the `hackpro.json` writer) — timer,
  shutdown, or a setting change. Confirming needs a runtime trace or a caller of the
  virtual slot at `0x180547890`.
- UNVERIFIED: whether `FUN_180042D70`'s second caller is a migration path or a
  re-read.
- UNVERIFIED: what `PlayLayer+0x3788` is — the field the `destroyPlayer` callback
  compares against before setting the cheat-attributed flag.
- UNVERIFIED: the exact meaning of setting tag `0x11D` (`getConfigVar<hackpro::Bytes>`)
  consumed by the onQuit path.
