# Native `Import .dt` action

- **Question** — how does Mega Hack's native **Import .dt** action work end to end?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — tag resolution

- `FUN_180002320` (`0x180002320`) registers both tabs and then performs the
  `"CORE"` readiness probe at `0x1800023DC` via `FUN_1800A2FB0`.
  See [tab-registration.md](tab-registration.md) and [entry-path.md](entry-path.md).
- The Import .dt action's tag is resolved only after core load; the boolean
  setter intercepts that exact tag and leaves every other setting untouched.
  The reconstruction implements this as action-tag interception in
  `Config::setHackEnabled` (`reconstruction/src/config.cpp`).

## Evidence — file picker

- `FUN_1800C6300` (`0x1800C6300`, 2460 bytes) wraps `GetOpenFileNameW`
  (confirmed at `0x1800C6300+...` in the decompiled function). It is the picker
  used by the Import .dt action.
- Cancelling the picker makes no changes: the action only proceeds after a file
  is selected.

## Evidence — label substitution

- `FUN_180098868` (`0x180098868`) is the label-substitution site referenced
  by the roadmap. It calls `FUN_180099460` and manipulates a language-keyed
  string, consistent with substituting the reused row's label while a language
  dictionary is loaded.
- The reused row is the former **Show Prefix** row: Mega Hack's packed interface
  has no spare row, so the Import .dt action takes that slot. See
  [nxui-menu.md](nxui-menu.md).

## Evidence — no-op on cancel

- The action first calls the `GetOpenFileNameW` wrapper. Only after a file is
  selected does it flush and invoke the importer; cancelling makes no changes.
  This is recorded in
  `samples/absolllute.hackmega/RE_NOTES_PERSISTENT_BEST_RUN.md` and is
  consistent with the picker wrapper's shape.

## UNVERIFIED

- The exact importer dispatch function. The roadmap names `0x180027C30`, but that
  address is the `hackpro::Bytes` setter (`setConfigVar<hackpro::Bytes>`), not a
  standalone importer. The setter is the interception point for the action tag,
  but the downstream importer dispatch still needs to be traced.
- The full end-to-end importer implementation (ZIP extraction, `general.dt`
  parsing, canonicalisation) lives in the sample patch's PowerShell tooling, not
  in the release binary.

## Conclusion

1. The Import .dt action is a native row reuse: the Show Prefix row's tag is
   resolved after core load, and the boolean setter intercepts it.
2. The file picker is `FUN_1800C6300` wrapping `GetOpenFileNameW`.
3. Label substitution happens at `0x180098868` during language dictionary load.
4. Cancellation is a no-op.
5. The importer dispatch address in the roadmap is mislabelled and needs
   correction.
