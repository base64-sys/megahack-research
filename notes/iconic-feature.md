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

**Install site.** The button is not gated by a settings key: `FUN_1800A8020`
(`0x1800A8020`) installs six UI hooks unconditionally, each through its own helper
(`FUN_1800A9E00`, `FUN_1800AA290`, `FUN_1800AA430`, `FUN_1800AA5D0`, `FUN_1800AA770`,
`FUN_1800AA950`) that calls `geode::hook::createConvention(TulipConvention)` and then
`FUN_180108E70(mod, target, detour, name, convention, &priority)` with priority
`0xFFFFFFE0` (-32):

| Target | RVA | Detour | Helpers |
| --- | --- | --- | --- |
| `AppDelegate::trySaveGame` | `0x83500` | `FUN_18010B000` | `FUN_1800A9E00` |
| `MenuLayer::init` | `0x333A90` | `FUN_18010B020` | `FUN_1800AA290` |
| `GJGarageLayer::init` | `0x273050` | `FUN_18010B060` (adds `"shards-menu"`) | `FUN_1800AA430` |
| `PauseLayer::customSetup` | `0x37C870` | `FUN_18010B400` | `FUN_1800AA5D0` |
| `PauseLayer::onResume` | `0x37E380` | `FUN_18010B4A0` | `FUN_1800AA770` |
| `ProfilePage::init` | `0x3BE840` | `FUN_18010CDE0` | `FUN_1800AA950` |

The same registrar then registers three settings callbacks with
`FUN_18008F910(registry, 0x1BD, …)`, `FUN_1800733D0(registry, 0x1BE, …)` and
`FUN_1800733D0(registry, 0x1BF, …)`, all pointing at `FUN_18010AFB0`, which reads
`FUN_18008F420(registry, 0x1BD)` and hands it to `FUN_180122C30` on the object from
`FUN_1800ADF90(FUN_1800ACA20())`. Those ids are the ICONIC row ids, not hack tags
([settings-registry.md](settings-registry.md)); none of them touches the
icon-stealer button, which is added on every `ProfilePage::init`.

**The gate is the original `init`.** `FUN_1801AA6E0` (`0x1801AA6E0`) is a lazy
trampoline for the host function it replaces — the same delayed-bind shape recorded for
other replaced host methods in [gameplay-hooks.md](gameplay-hooks.md): it resolves
`GetModuleHandleA(NULL)` + RVA `0x3BE840` once (`FUN_18010DDC0` / `FUN_1801AA7B0`,
guard `DAT_180810C50`, pointer `DAT_180810C48`) and tail-calls it with the caller's
registers untouched. The detour's own bytes show the check:

```
0x18010CDF7  mov   esi, r8d
0x18010CDFA  mov   rdi, rcx
0x18010CDFD  call  0x1801AA6E0     ; original ProfilePage::init
0x18010CE02  mov   ebx, eax
0x18010CE04  xor   al, 1
0x18010CE06  or    al, sil
0x18010CE09  jne   0x18010CF05     ; leave unless init() == true and r8b == 0
```

so "the gate returns 1" is "the original `ProfilePage::init` succeeded", and the
`param_3` in the decompiled signature is the third argument of the Tulip convention,
not an argument of `init`.

**Conclusion.** Icon Stealer is a `left-menu` button on the user-profile layer
(item id `icon-stealer`, sprite `GJ_selectSongBtn_001.png`), added inside the
`ProfilePage::init` detour whenever the original init succeeded, that offers a
confirmation popup and then writes the viewed
record's eight icon ids, three colours, glow byte and one extra dword onto the GD
host object at host RVA `0x17B4A0`. It is an action, not a toggle, so it has no
settings tag to promote against.

## Open questions

- UNVERIFIED: what the Tulip convention's third argument (the `r8b` the detour tests)
  carries; the detour leaves unless it is 0. Closing this needs the Geode
  `TulipConvention` definition pinned to the 5.6.1 baseline (references/) rather than a
  guess from the call site.
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

## Evidence - the particle preview chain (2026-10-28)

Two changelog rows name particle customisation - v7.0 (Full) "Added Icon Particle
Customisation" and v9.0.0 "Added Particle customisation" - and neither has a settings tag.
This section records the only particle-building chain in the image, so the next pass starts
from a measured shape instead of a name.

**Method.** A string search for `particles` in `recovered/functions/_xrefs.tsv` gives one
format literal, `{}-particles-{}`; the byte search over the DLL gives its exact VA and the two
plist names. `_callers.tsv` then walks the chain upwards to its root.

| Literal | VA (`.rdata`) |
| --- | --- |
| `{}-particles-{}` | `0x1805854F3` |
| `dashEffect.plist` | `0x180575B0B` |
| `dragEffect.plist` | `0x180575B37` |
| `game_bg_01_001.png` | `0x180571B60` |

The chain (all three functions in `.text`, extents from the decompiled headers):

| Function | Size | Role |
| --- | --- | --- |
| `FUN_1801220C0` (`0x1801220C0`) | 947 bytes, **0 callers** | builds the preview layer: `game_bg_01_001.png` layers, clipping node, then calls the kind dispatcher |
| `FUN_1801ED6C0` (`0x1801ED6C0`) | 140 bytes, 1 caller | `switch (kind)`, forwards to the node factory |
| `FUN_1801EE0F0` (`0x1801EE0F0`) | 702 bytes, 1 caller | creates the particle node and names it |

`FUN_1801EE0F0(kind, variant, path_prefix)` reads:

- `kind == 0xD` -> plist `dashEffect.plist`, id suffix `dash`;
- `kind == 0xC` -> plist `dragEffect.plist`, id suffix `base`;
- then `FUN_180240DE0(plist, variant)` returns the node, and
  `FUN_18000AE30(out, fmt, suffix, variant)` formats `{}-particles-{}` - the id becomes
  `dash-particles-<variant>` or `base-particles-<variant>` - which is stored with
  `cocos2d::CCNode::setID`.

**Conclusion.** Particle customisation is a *preview* surface: a layer builds a small stage,
then asks the dispatcher for a dash or base particle node named
`<kind>-particles-<n>`. Nothing in the chain reads a config tag (no `mov edx,<tag>` followed by
a registry call appears in any of the three bodies), so the feature is driven by the UI that
owns the preview, not by a boolean. Both rows stay `cluster` until that owner is identified.

**Open questions.**

- UNVERIFIED: which class owns `FUN_1801220C0`. It has no direct caller, so it is reached
  through a vtable or a callback table; the owner would name the page and the selection state.
- UNVERIFIED: what `FUN_180240DE0` is (read here as the particle-node constructor taking the
  plist and a variant index). Its name and argument list are not established.
- UNVERIFIED: whether the preview is read-only or writes the chosen particle set into the
  profile; no `setConfigVar` call appears in the three bodies, but the UI callback that
  receives the selection was not traced.
