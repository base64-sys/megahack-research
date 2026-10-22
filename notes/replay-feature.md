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
