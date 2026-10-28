# Iconic feature cluster

- **Question** — which code and definitions implement Mega Hack's Iconic
  (garage icon effects) features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — tab construction

- `FUN_18011FFE0` (`0x18011FFE0`) builds the `ICONIC` tab. It repeatedly
  calls `hackpro::define` for prefixes such as `ICONIC/DISABLE`,
  `ICONIC/VANILLA_PROGRESS_BAR`, `ICONIC/GENERAL`, `ICONIC/EFFECTS`,
  `ICONIC/WAVE`, `ICONIC/RAND`, then opens sections and adds numeric rows.
  See [definition-keys.md](definition-keys.md).
- `ICONIC/VANILLA_PROGRESS_BAR` is one of the keys used in that tab.

## Evidence — special icon effects

- `FUN_1801FC6C0` (`0x1801FC6C0`) builds icon effect UI and references:
  - `ICONIC/SPECIAL/PREVIEW`
  - `ICONIC/SPECIAL/INVERTED`
  - `ICONIC/SPECIAL/SPEED/PRE`
  - `ICONIC/SPECIAL/SPEED/SUF`
- `FUN_1801261C0` (`0x1801261C0`) references `ICONIC/PREVIEW/ICONS`.

## Evidence — shortcut registration

- `FUN_18003C470` and `FUN_1800B6770` register the `HOME/SHORTCUT/ICONIC`
  action, so Iconic can be reached from the shortcut system as well as from its
  own tab.

## Evidence — assets

- Iconic uses shipped sprites such as `iconic.png`, `iconic_btn.png`,
  `iconic_ghost_btn.png` and `iconic_particles_btn.png` (listed in `mod.json`).
  Those are Absolllute's assets and are not reproduced here.

## Evidence — Icon Stealer (2026-10-28)

The v9.1.0 "Added Icon Stealer" row is a **profile-page button**, not a settings
toggle: no `ICONIC/...` or `HACK_...` key gates it, and the feature is entered by
adding a sprite to a menu.

| Step | Address / value | Evidence |
| --- | --- | --- |
| Button installer | `FUN_18010CDE0` (`0x18010CDE0`, RVA `0x10CDE0`) | takes `(layer, unused, param_3)`; gate `FUN_1801AA6E0() == 1 && param_3 == 0` |
| Host menu | `CCNode::getChildByID([layer + 0x240], "left-menu")` | string built as an SSO pair at `local_60`/`local_58` |
| Sprite | `CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png")`, opacity/scale `0x3F4CCCCD` (0.8f) through vtable `+0xA0` | — |
| Menu item | `FUN_1801E6D10(sprite, layer, FUN_18010CF50)` | callback pointer is `0x18010CF50` |
| Item id | `"icon-stealer"` (`0x6574732D6E6F6369` = `icon-ste`, `0x72656C61` = `aler`), set by `FUN_18010D010` | SSO pair in the body |
| Insertion | `[left-menu vtable + 0x1C0](left-menu, item)` then `CCNode::updateLayout(left-menu, true)` | vtable call site |
| Popup | `geode::createQuickPopup("Icon Stealer", "Do you want to apply this users icons?", "Cancel", "Steal Icons", &lambda, 1, 0)` in `FUN_18010CF50` | `"Icon Stealer"` is `.rdata` `0x18056CFD9` and the only xref is `FUN_18010CF50` (`_xrefs.tsv`) |
| Confirm body | `FUN_18010D1A0` (`0x18010D1A0`) | the lambda blob at `.rdata` `0x18056CF58` starts with the pointer `0x18010D1A0` (raw qword read of the file) |

`FUN_18010CF50` first retains `[layer + 0x2B0]` — the clicked user's profile record —
and captures it in the popup callback.

On confirm (`*param_3 == 1`, i.e. the second "Steal Icons" button) `FUN_18010D1A0`
reads the captured record and copies it onto the GD host object returned by
`FUN_180147BA0` (the lazy resolver for host RVA `0x17B4A0`, see
[gd-structures.md](gd-structures.md#the-two-offsets-share-one-object-and-the-release-never-compares-them)):

| Source (profile record) | Setter | Destination (host object) |
| --- | --- | --- |
| `+0x2B0`, `+0x2B4`, `+0x2B8`, `+0x2BC`, `+0x2C0`, `+0x2C4`, `+0x2C8`, `+0x2CC` (8 icon ids) | `FUN_1801312B0(context, 0..7, 0, value)` | `+0x2C4/+0x2CC`, `+0x2D0/+0x2D8`, `+0x2DC/+0x2E4`, `+0x2E8/+0x2F0`, `+0x2F4/+0x2FC`, …, written as `value + base` |
| `+0x1B0`, `+0x1B4`, `+0x1B8` (3 colours) | `FUN_1801317C0(context, 0..2, 0, value)` | colour triples |
| `+0x2D8` (byte) | `FUN_1801321A0(context, byte, 0)` | glow |
| `+0x2DC` | `FUN_180131C70(context, 2, 0, value)` | `+0x35C` run ([gd-structures.md](gd-structures.md#0x35c--5-instruction-sites-in-3-functions-all-on-the-gd-host-object)) |

The first setter argument is the context object from `FUN_18012C690` (`0x18012C690`,
magic static at `.data` `0x180806C28`, 62 callers); it is only read at `+0x1C`, and
`FUN_1801312B0` skips the write when that dword is `1` and its own `param_3` is
non-zero. The record is released at the end (`cocos2d::CCObject::release`), so
nothing of the viewed user is retained after the copy.

**Conclusion.** Icon Stealer is a `left-menu` button on the user-profile layer
(item id `icon-stealer`, sprite `GJ_selectSongBtn_001.png`), gated by
`FUN_1801AA6E0()`, that offers a confirmation popup and then writes the viewed
record's eight icon ids, three colours, glow byte and one extra dword onto the GD
host object at host RVA `0x17B4A0`. It is an action, not a toggle, so it has no
settings tag to promote against.

## Open questions

- UNVERIFIED: what `FUN_1801AA6E0()` tests. It is called at the top of the installer,
  returns 1 for the normal case, and has three call sites (`0x18010DDC0`, `0x1801AA7B0`,
  `0x1803A5140` in `_calls.tsv`); the first of those is in the same profile-page
  cluster and is the cheapest place to close it.
- UNVERIFIED: which class the `left-menu` owner belongs to and what `[layer + 0x240]`
  and `[layer + 0x2B0]` are named. The `"left-menu"` / `"shards-menu"` node ids put
  the installer next to the profile-page builders in the `0x18010B000`–`0x18010D000`
  range, but no note names the layer.
- UNVERIFIED: whether `FUN_18012C690`'s `+0x1C` mode is the "preview only" switch that
  makes Icon Stealer stop at a preview instead of writing the host object.
- UNVERIFIED: which settings keys gate each Iconic effect. No `ICONIC/...`
  setting keys appear in the observed `hackpro.json` sample, so they may be
  definition-only or loaded from a different profile.
- UNVERIFIED: the exact render path for each effect (particles, ghost trail,
  glow, wave trail). `FUN_1801FC6C0` builds the UI; the gameplay/render hooks
  are not yet tied to individual toggles.
- UNVERIFIED: whether `ICONIC/DISABLE` disables the whole feature or only the
  preview.
