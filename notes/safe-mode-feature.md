# Safe Mode feature cluster

- **Question** — how does Mega Hack's Safe Mode / cheat safety work?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — exported API

From [hackpro-api.md](hackpro-api.md):

| VA | Demangled |
| --- | --- |
| `0x18019B350` | `mega::utils::safe_mode::areCheatsEnabled()` |
| `0x18019B450` | `mega::utils::safe_mode::disableAllCheats()` |
| `0x18019B5A0` | `mega::utils::safe_mode::hasCheatedInAttempt()` |

## Evidence — shared gate

From [hook-dispatch.md](hook-dispatch.md):

- `FUN_18019B5F0` (`0x18019B5F0`) is the shared predicate consulted by the
  `resetLevel`, `levelComplete` and `savePercentage` hooks.
- It checks for a current level, then three conditions:
  1. config tag `0x1E2` (`0x18008F420` / `getConfigVar<bool>`);
  2. config tag `0x1E1` plus cached safe-mode flag `DAT_18080CBD8`;
  3. config tag `0x49` (`getFlag`) plus `FUN_180199520() >= 1.0`.
- `FUN_18019B330` (`0x18019B330`) caches
  `mega::utils::safe_mode::areCheatsEnabled()` into `DAT_18080CBD8` and runs
  after every `resetLevel`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

- `CHEAT_SAFETY/HACK_AUTO_SAFE_MODE`
- `CHEAT_SAFETY/HACK_SAFE_MODE_POPUP`
- `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE`

## Evidence — progress accounting

- `FUN_1801992B0` (`0x1801992B0`) accumulates frame time into
  `DAT_18080CB70` and, when `DAT_18080CB68 == 1`, into `DAT_18080CB78`.
- `FUN_180199520` returns `1.0 - DAT_18080CB78 / DAT_18080CB70` when the
  denominator is positive, otherwise `1.0`.
- The gate fires when the cheat-attributed share of total progress reaches
  `1.0`.

## Pattern

Safe Mode is a cross-cutting gate, not a feature toggle: most gameplay hooks
consult `FUN_18019B5F0()` before acting. `disableAllCheats()` is called from
the `resetLevel` hook path (`FUN_180199E80` -> `FUN_18019B330`) and from a
settings setter (`FUN_180027C30` inside `FUN_180090BC0`), so cheat disable is
triggered by a config write as well as by level start.

## Open questions

- UNVERIFIED: which config keys correspond to tags `0x1E1`, `0x1E2` and `0x49`.
- UNVERIFIED: what sets `DAT_18080CB68` to `1`.
- UNVERIFIED: what `FUN_180199520()`'s progress ratio measures in player-facing
  terms.
- UNVERIFIED: which callers of `areCheatsEnabled` / `disableAllCheats` are the
  ranked-play gate.
- UNVERIFIED: whether Safe Mode state persists across levels or resets per
  attempt.
