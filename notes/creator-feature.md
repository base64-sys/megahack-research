# Creator feature cluster

- **Question** — which settings and code implement Mega Hack's creator features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

- `CREATOR/HACK_ACCURATE_SAVE`
- `CREATOR/HACK_COPY_HACK`
- `CREATOR/HACK_CUSTOM_OBJECT_BYPASS`
- `CREATOR/HACK_EDITOR_EXTENSION`
- `CREATOR/HACK_SMOOTH_EDITOR_TRAIL`
- `CREATOR/HACK_TRIGGER_VALUE_BYPASS`
- `CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/TRANSPARENCY`

## Evidence — UI registration

- `FUN_180062940` (`0x180062940`) registers:
  - `HACK_CUSTOM_OBJECT_BYPASS`
  - `HACK_EDITOR_EXTENSION`
- Changelog features include:
  - "Slider Bypass now supports scale slider"
  - "Fixed Slider Bypass conflicting with BetterEdit"
  - "Fixed StartPos Quick Switch showing on Android"

## Evidence — editor hooks

From [ui-addons.md](ui-addons.md) and [hook-dispatch.md](hook-dispatch.md):

- `LevelEditorLayer::init` is hooked three times
  (`0x180151280`, `0x1801601F0`, `0x1801812F0`).
- `LevelEditorLayer::postUpdate` is hooked once (`0x180181540`).
- `LevelEditorLayer::updateVisibility` is hooked once (`0x1801582C0`).
- `LevelEditorLayer::onPlaytest` and `onStopPlaytest` are hooked.
- `EditorPauseLayer::init`, `saveLevel`, `onSaveAndPlay`,
  `onSaveAndExit` and `onExitNoSave` are hooked.
- `EditorOptionsLayer::onButtonRows` and `onButtonsPerRow` are hooked.

## Evidence — Smooth Editor Trail (2026-10-25)

`HACK_SMOOTH_EDITOR_TRAIL` (tag 55, `0x37`, static label map entry #67, store VA
`0x180064222`) is registered at `0x1800B058D`; callback `FUN_180163EC0` hooks
`LevelEditorLayer::postUpdate` (RVA `0x2E0F40`, name string from the `lea r9` operand)
with detour `0x180163F30`, installed by `0x180158CC0`.

The detour is two instructions before the original:

```
0x180163F30  mov  dword [rcx+0x3898], 0x3D088889   ; 0.033333335f = 1/30 s
0x180163F3A  jmp  0x18019FFD0                       ; original postUpdate
```

So the toggle sets the editor's trail sample interval field at `LevelEditorLayer+0x3898` to
exactly 1/30 s and then runs the unmodified `postUpdate`. That is a smoothing interval, not a
suppression: the trail refresh is pinned to 30 Hz instead of whatever GD computes per frame.

UNVERIFIED: the field's declaration/type (`+0x3898` is inferred from this single store), what
GD writes there normally, and whether `MULTIPLE_EDITOR_TRAILS` (tag 55, `0x37`) is what sets
`+0x3898` when this toggle is off — the adjacency of the two tags in the static map is not
evidence of a shared field.

## Open questions

- UNVERIFIED: which settings tags correspond to each creator hack.
- UNVERIFIED: whether `HACK_EDITOR_EXTENSION` is a feature toggle or an
  integration hook.
- UNVERIFIED: whether `HACK_CUSTOM_OBJECT_BYPASS` and
  `HACK_TRIGGER_VALUE_BYPASS` veto or modify editor operations.
- UNVERIFIED: whether `HACK_ACCURATE_SAVE` changes `LevelEditorLayer::saveLevel`
  or GD's save path.
