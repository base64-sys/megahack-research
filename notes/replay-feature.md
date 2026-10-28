# Replay feature cluster

- **Question** — which settings and code implement Mega Hack's Replay features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

| Setting key | Observed value | Role |
| --- | --- | --- |
| `REPLAY/GAMEPLAY/ALLOW_RANDOMNESS` | False | Allow randomness in replay |
| `REPLAY/GAMEPLAY/BLOCK_INPUTS` | True | Block inputs during replay |
| `REPLAY/SAVE_LAYOUT/AUTO_SAVE` | True | Auto-save replay layout |

## Evidence — UI registration

- `FUN_180062940` (`0x180062940`) registers `HACK_REPLAY_LAST_CHECKPOINT`.
- Changelog features include:
  - "Replay now supports practice Dash orb"
  - "Replay accuracy improved"
  - "Replay convert failing on valid replays"
  - "Replay not supporting non-singleton game layers"
  - "Replay option to block inputs"
  - "Replay Open Folder"
  - "Improved Replay accuracy"

## Evidence — messages

- `MESSAGE/REPLAY/CONFIRM_CLEAR`
- `MESSAGE/REPLAY/CONFIRM_DELETE`
- `MESSAGE/REPLAY/CONVERT_SUCCESS`
- `MESSAGE/REPLAY/SAVE`
- `ERROR/REPLAY/EMPTY_FILENAME`
- `ERROR/REPLAY/FILE_NOT_FOUND`
- `ERROR/REPLAY/NO_INPUTS`

## Evidence — hooks

- `PlayLayer::storeCheckpoint` (`0x180176250`) and
  `PlayLayer::createCheckpoint` (`0x18017B580`) are hooked; see
  [save-data-hooks.md](save-data-hooks.md).
- `PlayerObject::pushButton` and `releaseButton` are hooked multiple times; see
  [input-hooks.md](input-hooks.md).

## Open questions

- UNVERIFIED: which functions read each Replay setting tag.
- UNVERIFIED: how replay data is stored and loaded (file format not identified
  in the release binary).
- UNVERIFIED: whether replay uses the checkpoint hooks or only observes them.
- UNVERIFIED: the exact semantics of "Allow Randomness" and "Block Inputs".

## Evidence - Replay Open Folder is action 0x375 (2026-10-28)

The v9.1.0-beta.6 changelog row "Added Replay Open Folder" is an action listener, not a
toggle and not a hook.

**Method.** `recovered/functions/_xrefs.tsv` was searched for `replay`; the only path-shaped
string is the SSO literal `replays`, whose single owner is `FUN_18019E360`. That function's
address is then looked up in the decompiled tree, which finds the registrar, and the two
callees are read to name the effect.

| Step | Evidence |
| --- | --- |
| registrar | `FUN_18019CA80` binds `FUN_180090F30(registry, 0x375, &fn)` with the function object holding `FUN_18019E360` (`recovered/functions/18019ca80_FUN_18019ca80.c`; the same block binds `0x374` to `FUN_18019E060`, the `MESSAGE/REPLAY/CONVERT_SUCCESS` action) |
| action id | `0x375` (885); `FUN_180090F30` = `registerActionListener`, FNV-1a-64 into `registry + 0x1D0` (see [ui-addons.md](ui-addons.md#evidence---crash-game-and-the-utilities-action-map-2026-10-28)) |
| body | `FUN_18019E360` (`0x18019E360`, RVA `0x19E360`) builds the SSO string `"replays"` (`0x6C706572` = `repl`, `0x737961` = `ays`), converts/joins it with `FUN_1800C6130`, then calls `FUN_1800C4DC0` |
| join | `FUN_18003F560` is `std::filesystem::path::operator/` (drive-letter test `(c & 0xFFFFFFDF) - 0x3A0041 < 0x1A`, `/` and `\` separators), and `FUN_1803A6ACC` returns the ANSI/UTF-8 file-API flag used for the wide-string conversion |
| effect | `FUN_1800C4DC0` is `ShellExecuteW(NULL, L"open", path, NULL, NULL, 10)` - `10` is `SW_SHOWDEFAULT`, i.e. an Explorer window on the folder |

**Conclusion.** Action `0x375` opens the mod's `replays` directory in the shell. The row is
`mapped` on the action id, its body, and the `ShellExecuteW` call; the folder it opens is only
as specific as the join base.

**Open questions.**

- UNVERIFIED: what base path `FUN_1800C6130` joins `replays` onto (mod directory vs the GD
  install directory). What would close it: the argument source in `FUN_180028700`, or a
  runtime observation of the Explorer window's path.
- UNVERIFIED: which settings page renders the `0x375` row; action ids carry no label, and the
  id-to-label binder is still unknown ([ui-addons.md](ui-addons.md#evidence---the-action-listener-registry-2026-10-28)).
