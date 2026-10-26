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

## Lead — Copy Hack's two LevelInfoLayer spoofs (2026-10-26)

`HACK_COPY_HACK` is a creator toggle (`CREATOR/HACK_COPY_HACK`) with a static label and two
read hooks, but no `Added ...` changelog bullet names it, so this is recorded as a lead: the
mechanism is fully read, the feature identity and the fields it spoofs are not.

| Field | Value |
| --- | --- |
| Tag | 43 (`0x02B`, `HACK_COPY_HACK`) |
| Static label | map entry 15, store `0x180062F43` |
| Registration | `0x1800B00DD` |
| Callback | `0x18015E920` |
| Hook 1 | `LevelInfoLayer::init`, RVA `0x2F7340` -> detour `0x18015E9F0`, installer `0x18015EAB0`, name `0x1805752BC` |
| Hook 2 | `LevelInfoLayer::levelDownloadFinished`, RVA `0x2FBA60` -> detour `0x18015EA50`, installer `0x18015ED00`, name `0x1805752D1` |

Both installers are called with order `0xFFFFFFE0` and the sixth argument `1`; the disable
branch resolves through `0x180108A80`. Hook identity is corroborated from the far side as
well: the trampoline initialisers contain the matching base displacements —
`0x1801A0ED9 lea rcx,[rax+0x2F7340]` inside `0x1801A0E60`, and
`0x1801A0A41 lea rcx,[rax+0x2FBA60]` inside `0x1801A09D0`.

### The spoof

Both detours treat the qword at `[level+0x3A8]` as two adjacent 32-bit fields and make their
difference exactly one for the duration of the vanilla call:

```
rsi = level (the second argument)
q   = [level+0x3A8]          ; low dword = +0x3A8, high dword = +0x3AC
if ((q.low - q.high) != 1):
    q.low = q.high + 1        ; store dword to [level+0x3A8]
call <cached original>
if ([level+0x3A8] - [level+0x3AC] != 1):
    [level+0x3A8] = q         ; restore both halves
```

`init` keeps the original's bool result (`test al,al; je return`), while
`levelDownloadFinished` falls through to the restore without preserving any return value.
Neither body touches a tag: the toggle is install/uninstall, like most of this build's hooks.

### Why this stays a lead

- No `Added ...` bullet in the 389 names a copy hack. The three copy-worded rows
  (`Added Show Password to Copy Level Screen`, `Added SongID Copy`, `Added Copy LevelID Fix`)
  describe different surfaces — a password label, a song ID, and a level-ID fix — and none of
  their cluster links lead to this callback, so binding tag 43 to one of them would be a
  guess. All three stay `cluster`.
- The two spoofed ints are host-side data with no name string in the mod image, so the
  intended effect of "difference 1" cannot be read here.

UNVERIFIED: what `[level+0x3A8]` and `[level+0x3AC]` are (the pair is only proven to be two
32-bit fields inside one qword); why the vanilla code treats a difference of one as special;
whether the effect is exposing the Copy button, making a non-copyable level copyable, or
something else entirely; and which changelog wording, if any, corresponds to this toggle.
