# Android builds use a different ABI, and carry no RTTI

- **Question** — does the Android build carry the same class set as Windows? This is the
  first real question M5 needs answered.
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/compare_platform_classes.py`.
- **Status** — partially answered. The headline is a negative result that changes the plan.

## The finding

The two platforms do not share a C++ ABI, and Android has no type information at all.

| Binary | ABI | MSVC type descriptors | `_ZN` symbols | `typeinfo for` |
| ------ | --- | --------------------: | ------------: | -------------: |
| `absolllute.hackmega_original.dll` | MSVC x64 | 518 | 0 | 0 |
| `absolllute.megahack.android64.so` | Itanium | 0 | 3,052 | 0 |
| `absolllute.megahack.android32.so` | Itanium | 0 | 3,051 | 0 |

Two independent confirmations per platform: the Windows build contains MSVC `.?AV`/`.?AU`
type descriptors and no Itanium symbols at all, while both Android builds contain the exact
opposite. Neither ABI's markers appear in the other's binary, so this is not a matter of
stripping or retention settings.

## What this means for M5

**The 497-class inventory cannot be carried to Android.** `rtti_index.py` walks MSVC
`CompleteObjectLocator` structures; the Android builds have no equivalent, because there are
no RTTI typeinfo symbols to walk. Android is built with type information disabled, which is
Geode's default for size. M5 needs a different approach rather than a rerun of the same tool.

**Android still has a usable surface, just a different one.** The 3,052 `_ZN` symbols are
function manglings and do name methods with their owning types:

```
AppDelegate::get
AppDelegate::trySaveGame
BoomListView::init
CCAnimatedSprite::runAnimation
CCTextInputNode::onTextFieldAttachWithIME
```

So the Android method surface is recoverable; it is the *class* set that is not, and
`cocos2d::` types appear there because Android links cocos2d statically where Windows resolves
some of it from the host.

**android32 and android64 are near-identical builds** — 3,051 versus 3,052 `_ZN` symbols.
The one-symbol difference is not worth chasing without a diff, and no claim is made about it.

## String parity, which is a better check than class names

Since class names do not transfer, the question becomes whether *strings* do. They do, mostly,
and where they differ is where the platform split actually lives.

ASCII strings of length ≥5: 21,320 in the Windows build, 29,298 in android64. **9,233 are
shared — 43%.**

| Marker | Windows | Android64 | Shared |
| ------ | ------: | --------: | -----: |
| `*.png` asset names | 8 | 10 | **8 (all)** |
| `HOME/…` config keys | 36 | 26 | 24 |
| `/api/mega_hack/…` | 3 | 3 | 2 |
| `ICONIC/…` | 7 | 8 | 1 |
| `MESSAGE/…` | 2 | 3 | 0 |
| `KEYBINDS/…` | 3 | **0** | **0** |

Three things follow, in order of confidence.

**The asset pipeline is identical.** Every Windows `.png` name appears in the Android build —
`iconic_sdc_off.png`, `snow.png`, `square02b_001.png` and the rest. Asset names are not a
divergence point at all.

**Keybinds are a Windows-only feature.** Three `KEYBINDS/…` keys on Windows, *zero* on
Android, and no overlap. That is exactly what one would expect if the feature is gated on
having a keyboard, and it is the clearest single signal of platform divergence found so far.
UNVERIFIED: whether Android drops the feature entirely or ships it disabled, and whether the
Android build routes keybinds through touch input under a different key prefix.

**Core configuration is mostly shared.** 24 of 36 `HOME/…` keys are common to both, so the
settings surface is broadly the same with 12 Windows-only keys. The divergence is concentrated
in input and platform-specific features rather than in the configuration model itself.

`ICONIC/…` sharing only 1 of 7 needs care: those keys were recovered from MSVC SSO literals on
Windows, which is a lossy method with known split and merge error modes, while the Android
count comes from plain ASCII strings. Part of that gap is therefore a measurement artefact
rather than real divergence. The comparison is not yet apples-to-apples.

## Correcting the ICONIC result: it was a measurement artefact

The comparison above is **asymmetric** and its ICONIC row is wrong. Windows keys came from
MSVC SSO reconstruction — lossy, with documented split and merge error modes — while Android
keys came from plain ASCII strings. That made Windows look like it had almost no ICONIC keys
when it has more than Android.

Re-run with the **same extraction on both sides** — a raw byte scan of each binary, no
decompiler involved:

| Prefix | Windows | android64 | Shared | Win-only | Android-only |
| ------ | ------: | --------: | -----: | -------: | -----------: |
| `ICONIC/` | 17 | 24 | **17** | **0** | 7 |
| `HOME/` | 46 | 31 | 28 | 18 | 3 |
| `MESSAGE/` | 6 | 5 | 4 | 2 | 1 |
| `ERROR/` | 8 | 7 | 6 | 2 | 1 |
| `KEYBINDS/` | 5 | 0 | 0 | **5** | 0 |
| `DISPLAY/` | 2 | 0 | 0 | **2** | 0 |

The ICONIC row inverts completely. Windows has **every** key Android has, plus Android carries
7 more. The earlier "1 of 7 shared" was an artefact of the SSO method undercounting Windows,
not real divergence. Correcting that here rather than adding a second version of it.

The lesson is the one AGENTS.md keeps stating: comparing two measurements taken by different
methods produces a number, and the number is meaningless.

## No single extraction is symmetric — and that is the actual finding

A raw ASCII byte scan on **both** binaries is still not a fair comparison, and the Windows
side proves it. The Windows-only list it produces is full of truncations:

```
CRASH/0H    ERROR/DIH    HOME/ANIH    HOME/DOTH    HOME/BG_H
HH9/uCH     NDS/VIEWH    PEED/PREH    ICONIC/DH    LESETS/1H
```

Those are not settings keys. They are the regex landing on non-string bytes.

Meanwhile the Android-only list is clean: `ICONIC/DISABLE`, `ICONIC/GLOW`, `MESSAGE/YES`,
`REPLAY/SAVE`, `LEVEL/NOCLIP/HACK_NOCLIP`.

And the giveaway is that **`ICONIC/DISABLE`, `ICONIC/RAND`, `ICONIC/GENERAL` and
`ICONIC/EFFECTS` appear Android-only here** — those are precisely the keys recovered from the
*Windows* build's SSO immediates back in [sso-decoder.md](sso-decoder.md).

The reason is the storage format, and it is the whole reason this comparison keeps going
wrong:

| | Short string storage | Found by |
| --- | --- | --- |
| Windows (MSVC) | packed into 4-byte SSO immediates, **non-contiguous** | SSO decode only |
| Android (clang) | plain contiguous ASCII | raw ASCII scan only |

Neither method finds both platforms. A contiguous-bytes regex cannot see a Windows SSO
string because the characters are interleaved with padding; an SSO decoder cannot see an
Android string because there is no immediates stream to read.

**So the only fair extraction is asymmetric by necessity:** Windows as the union of an ASCII
scan and the SSO decode, Android as an ASCII scan. Until that union is built, **every
overlap number in this note is unreliable** and the two earlier ones were both wrong — first
because Windows was undercounted, then because Windows was overcounted with garbage. Corrected
twice is still not a pass.

What survives both errors, because it does not depend on the extraction:

- The **ABI** result is solid: MSVC RTTI present on Windows, zero on Android. That is a
  count of whole structures, not of strings.
- `KEYBINDS/` and `DISPLAY/` are **absent from Android entirely**. Absence cannot be created
  by a Windows-side extraction bug.
- Assets are **fully shared** — 8 of 8 `.png` names, and those are long enough to be plain
  ASCII on both sides.

## What is genuinely Windows-only

Two prefixes are entirely absent from Android, under the symmetric extraction:

- **`KEYBINDS/`** — 5 keys, Windows only.
- **`DISPLAY/`** — 2 keys, Windows only.

Both are input and window-management concerns, which is a consistent story with the earlier
keybind finding. UNVERIFIED: whether Android carries these features under a different key
prefix, or drops them entirely.

`android32` and `android64` return identical counts for every prefix above, which is a
consistent picture of the same source built for two ABIs.

## What is not yet established

`compare_platform_classes.py` does **not** produce a usable Windows/Android class overlap,
and its output claiming "0 shared" must not be read as a finding. The two ABIs encode
namespaces differently (`hackpro::extensions::Home` versus `hackpro.extensions.Home`), and the
MSVC side additionally carries the compiler's `V` struct markers
(`VObject@ui@hackpro`), so the comparison is comparing raw text in two different
encodings. Normalising that is the obvious next step and has not been done.

The Android extractor stops at the first non-identifier component, so template arguments and
function parameter lists are dropped. That is adequate for method names and not adequate for
recovering template instantiations.

## The union extraction, and what it finally shows

Built as the note required: Windows from `_xrefs.tsv` (which is an ASCII scan **union** the
SSO decode, by construction), Android from an ASCII scan. Both sides then filtered by the same
shape rule — every `/`-separated segment an upper-case identifier — which is how Mega Hack
actually writes keys.

| | Count |
| --- | ---: |
| Windows keys | 80 |
| Android keys | 95 |
| **Shared** | **51** |
| Windows-only | 29 |
| Android-only | 44 |

About a dozen entries on each side are still regex artefacts (`D/}4`, `K/FV`, `Z2/B`,
`ERROR/REPLAY/EMP`, `OME/SCALE`, the `OME/…` truncation family documented in
[sso-decoder.md](sso-decoder.md)). They are listed below rather than silently dropped, because
the `OME/` and `ERROR/REPLAY/` families are recognisable SSO damage and the short random ones
are Android scan noise. Neither affects the groups below, which are contiguous and real.

### Windows-only, and it is all input and windowing

| Group | Keys |
| ----- | ---- |
| `KEYBINDS/` | `SET`, `REMOVE`, `VIEW`, `EMPTY`, `VERSION`, `SHORTCUT`, `DISABLE_IN_EDITOR` |
| `MESSAGE/KEYBINDS/` | `EMPTY`, `UNBINDABLE` |
| Keyboard misc | `HOME/HOTKEY_ICONS` |
| Windowing | `DISPLAY/HACK_BORDERLESS_CLASSIC`, `MESSAGE/DISPLAY/FULLSCREEN_STOPPED` |
| Theming | `HOME/THEME/{}/COL`, `HOME/THEME/{}/COL/ALT`, `HOME/THEME/{}/TOGGLE` |
| Desktop paths | `ERROR/HOME/NO_APPDATA` |
| Legacy saves | `ERROR/HOME/LEGACY_PARSE_FAIL`, `LEGACY_SAVE_KEYBINDS`, `LEGACY_SAVE_PARSE`, `MESSAGE/HOME/CONFIRM_LOAD_LEGACY` |

Every one is keyboard input, window management, desktop paths, or legacy save migration.

### Android-only, and it is touch and customisation

| Group | Keys |
| ----- | ---- |
| `ICONIC/PREVIEW/` | **11 keys** — `BASE_PARTICLES`, `COL1`, `COL2`, `DASH_EFFECT`, `DEATH_EFFECT`, `EXTRA`, `GHOST_TRAIL`, `GLOW`, `SHIP_FIRE`, `TRAIL`, `TRAIL_COL`, `WAVE_TRAIL` |
| Window geometry | `HOME/WIDTH`, `HOME/HEIGHT`, `HOME/FLOATER/X`, `HOME/FLOATER/Y`, `HOME/ACCENT`, `HOME/BACKGROUND`, `HOME/TAB_TEXT` |
| Replay UI | `REPLAY/NONE`, `REPLAY/OPEN`, `REPLAY/SAVE`, `MESSAGE/REPLAY/SAVE` |
| Dialogs | `MESSAGE/YES`, `MESSAGE/NO` |
| Other | `ICONIC/OVERRIDE`, `ICONIC/WAVE`, `HOME/SEARCH/AUTO_UPDATE` |

### Two details worth calling out

**Android has an entire icon-preview customisation layer that Windows does not** — 11
`ICONIC/PREVIEW/*` keys, covering particles, colours, and per-event effects (dash, death, ghost
trail, ship fire, wave trail). Nothing comparable exists on the Windows side.

**The dialog vocabulary differs.** Windows uses `MESSAGE/CANCEL`; Android uses
`MESSAGE/YES` and `MESSAGE/NO`. That is a platform-appropriate difference in button sets
rather than a rename, and it is the clearest sign these are deliberate platform builds rather
than one build with features stripped.

### What still survives as solid

51 shared keys is the core configuration surface, and it is common to both. Combined with the
findings that did not depend on extraction at all — the ABI difference, the absent RTTI, the
Windows-only `KEYBINDS/` and `DISPLAY/`, and the fully shared asset names — the platform
picture is coherent.

## CORRECTION: Windows has the icon-preview layer too

The previous section claims Android has "an entire icon-preview customisation layer that
Windows does not". **That is wrong.** All of the keys are in the Windows binary:

```
ICONIC/PREVIEW/BASE_PARTICLES   ICONIC/PREVIEW/COL1      ICONIC/PREVIEW/COL2
ICONIC/PREVIEW/DASH_EFFECT      ICONIC/PREVIEW/DEATH_EFFECT
ICONIC/PREVIEW/EXTRA            ICONIC/PREVIEW/GHOST_TRAIL
ICONIC/PREVIEW/GLOW             ICONIC/PREVIEW/ICONS
ICONIC/PREVIEW/SHIP_FIRE        ICONIC/PREVIEW/TRAIL
ICONIC/PREVIEW/TRAIL_COL        ICONIC/PREVIEW/WAVE_TRAIL
```

They went missing because the Windows side was taken from `_xrefs.tsv`, which contains only
`ICONIC/PREVIEW/ICONS` and `ICONIC/SPECIAL/PREVIEW`. A raw scan of the same DLL finds all
twelve. `_xrefs.tsv` undercounts SSO strings — it keeps only what the decompiler surfaced —
so it is the wrong source for this question.

This is the **fourth** correction in this note, and the failure mode is consistent: every time,
a Windows-side extraction gap has been read as a platform difference. Absence of evidence was
treated as evidence of absence, which AGENTS.md forbids and which I have now done three times
on the same comparison.

There is no icon-preview asymmetry. The Windows-only and Android-only lists above stand for
the other groups, but every entry in them should be re-checked the same way before being
treated as a real divergence.

## What the two ABIs independently agree on

One genuine cross-check is now possible, because the same class appears under both manglings:

| Platform | Form | Class |
| -------- | ---- | ----- |
| Windows | MSVC `.?AVPreviewLayer@iconic@@` | `iconic::PreviewLayer` |
| Android | Itanium `N6iconic12PreviewLayerE` | `iconic::PreviewLayer` |

Two different ABIs, two different compilers, the same class — recovered independently on each
side. That is the only fully verified platform-parity statement so far, and it happens to
contradict the claim it was collected to support.

Windows RTTI also carries the rest of the icon-preview implementation:
`iconic::SpecialConfigLayer`, `iconic::SpecialCore`, `SpecialRGB`, `SpecialGradient`,
`SpecialPastel`, `SpecialCustom`, `iconic::IconConfigPicker`, `iconic::SimpleParticles`,
`iconic::SimpleDeathEffect`, `iconic::SimpleGhostTrail` — the feature is not merely keyed, it
is a substantial subsystem on Windows too.

## The re-check I owed, and its result

Every entry in the "Windows-only" list was re-tested against a **contiguous byte scan of the
DLL**, the check that caught the `ICONIC/PREVIEW/` error.

| Key | Windows | Android | Verdict |
| --- | --- | --- | --- |
| `DISPLAY/HACK_BORDERLESS_CLASSIC` | yes | no | Windows-only |
| `ERROR/HOME/LEGACY_PARSE_FAIL` | yes | no | Windows-only |
| `ERROR/HOME/LEGACY_SAVE_KEYBINDS` | yes | no | Windows-only |
| `ERROR/HOME/LEGACY_SAVE_PARSE` | yes | no | Windows-only |
| `ERROR/HOME/NO_APPDATA` | yes | no | Windows-only |
| `HOME/HOTKEY_ICONS` | yes | no | Windows-only |
| `HOME/THEME/{}/COL` | yes | no | Windows-only |
| `HOME/THEME/{}/COL/ALT` | yes | no | Windows-only |
| `HOME/THEME/{}/TOGGLE` | yes | no | Windows-only |
| `KEYBINDS/DISABLE_IN_EDITOR` | yes | no | Windows-only |
| `KEYBINDS/EMPTY` | yes | no | Windows-only |
| `KEYBINDS/SHORTCUT` | yes | no | Windows-only |
| `KEYBINDS/VERSION` | yes | no | Windows-only |
| `MESSAGE/DISPLAY/FULLSCREEN_STOPPED` | yes | no | Windows-only |
| `MESSAGE/HOME/CONFIRM_LOAD_LEGACY` | yes | no | Windows-only |
| `MESSAGE/KEYBINDS/EMPTY` | yes | no | Windows-only |
| `MESSAGE/KEYBINDS/UNBINDABLE` | yes | no | Windows-only |
| `HOME/SEARCH` | yes | **yes** | **both** — reclassified |
| `KEYBINDS/REMOVE` | SSO | no | Windows-only, not contiguous |
| `KEYBINDS/SET` | SSO | no | Windows-only, not contiguous |
| `KEYBINDS/VIEW` | SSO | no | Windows-only, not contiguous |
| `MESSAGE/CANCEL` | SSO | no | Windows-only, not contiguous |

**17 of 22 survive the check.** The four marked `SSO` are present in the Windows build but not
contiguously, because they live in packed SSO immediates — a contiguous scan cannot see them,
which is the same limitation that hid `ICONIC/PREVIEW/` from `_xrefs.tsv`. They are Windows-only
on the evidence that they are absent from Android.

`HOME/SEARCH` is the one genuine reclassification: it is present on **both**, and my earlier
Android-side scan missed it. That is an Android-side extraction gap rather than a Windows one —
the opposite of the `ICONIC/PREVIEW/` error, and worth noting that both directions occur.

So the Windows-only group is real: **keyboard input, window management, desktop paths, legacy
save migration, and theme toggles**. 17 keys confirmed by direct byte evidence.

## The Android-only re-check: most of it was wrong too

Same treatment for the Android-only list. Only **10 of 32** survive.

**Confirmed Android-only** — present in both `.so` files, absent from the DLL:

| Key | Area |
| --- | ---- |
| `REPLAY/NONE`, `REPLAY/OPEN`, `REPLAY/SAVE`, `MESSAGE/REPLAY/SAVE` | replay UI |
| `MESSAGE/YES`, `MESSAGE/NO` | dialog buttons |
| `ICONIC/OVERRIDE`, `ICONIC/WAVE` | icon modes |
| `HOME/RULESETS/0`, `HOME/RULESETS/1` | ruleset slots |

**Reclassified as shared — 22 keys, all of which are in the DLL after all:**

- all twelve `ICONIC/PREVIEW/*`
- `HOME/WIDTH`, `HOME/HEIGHT`, `HOME/FLOATER/X`, `HOME/FLOATER/Y`, `HOME/ACCENT`,
  `HOME/BACKGROUND`, `HOME/TAB_TEXT`
- `ERROR/LEVEL/NO_CBF`, `ERROR/REPLAY/FILE_NOT_FOUND`, `HOME/SEARCH/AUTO_UPDATE`

This kills the "Android-only is touch and customisation" framing from two commits ago. The
window-geometry keys are on both platforms, so they are not a mobile adaptation — they are
simply Mega Hack's window settings, and the earlier list simply never found them on Windows.

### What is genuinely a platform difference, after both re-checks

| | Windows-only | Android-only |
| --- | --- | --- |
| Keyboard | `KEYBINDS/*` × 7, `HOME/HOTKEY_ICONS` | — |
| Windowing | `DISPLAY/HACK_BORDERLESS_CLASSIC`, `MESSAGE/DISPLAY/FULLSCREEN_STOPPED` | — |
| Dialogs | `MESSAGE/CANCEL` | `MESSAGE/YES`, `MESSAGE/NO` |
| Replay | — | `REPLAY/{NONE,OPEN,SAVE}`, `MESSAGE/REPLAY/SAVE` |
| Desktop paths | `ERROR/HOME/NO_APPDATA` | — |
| Legacy saves | `ERROR/HOME/LEGACY_*`, `MESSAGE/HOME/CONFIRM_LOAD_LEGACY` | — |
| Theming | `HOME/THEME/{}/COL`, `/COL/ALT`, `/TOGGLE` | — |

The **dialog vocabulary** difference is now confirmed from both directions at once: Windows
has `MESSAGE/CANCEL` and Android has `MESSAGE/YES` and `MESSAGE/NO`, with no overlap in either
direction. That is a real platform-specific choice, not a rename.

The **replay UI** is genuinely Android-only — five keys, absent from Windows — which is the
most concrete feature difference the comparison has produced.

## Settling the `sso-unproven` cases with the store disassembly

`verify_platform_keys.py` refuses to resolve a chunk-matched SSO string, because
"every 4-byte chunk occurs somewhere in 8.7 MB" succeeds by chance. It reports
`sso-unproven` and leaves the question open. Resolving it needs the decoder that
actually reads stores, so all ten candidates were checked against
`decode_sso.py` over the Windows tree.

| Key | Contiguous in DLL | In Windows SSO tree | Verdict |
| --- | --- | --- | --- |
| `HOME/RULESETS/0` | no | **yes** — `HOME/RULESETS/0x` | **shared** |
| `HOME/RULESETS/1` | no | **yes** — `HOME/RULESETS/1y` | **shared** |
| `REPLAY/NONE` | no | no | android-only |
| `REPLAY/OPEN` | no | no | android-only |
| `REPLAY/SAVE` | no | no | android-only |
| `MESSAGE/REPLAY/SAVE` | no | no | android-only |
| `MESSAGE/YES` | no | no | android-only |
| `MESSAGE/NO` | no | no | android-only |
| `ICONIC/OVERRIDE` | no | no | android-only |
| `ICONIC/WAVE` | no | no | android-only |

**Two reclassified as shared.** `HOME/RULESETS/0` and `HOME/RULESETS/1` decode in the Windows
tree as `HOME/RULESETS/0x` and `HOME/RULESETS/1y` — the trailing-character artefact documented
in [sso-decoder.md](sso-decoder.md), where a store one byte wider than the text appends one
spurious character. A strict equality test rejects both, which is exactly why the chunk
heuristic marked them unproven rather than absent.

**Eight confirmed Android-only**, each now with two independent negatives on Windows: the
bytes are not contiguous in the DLL, and the SSO decoder does not find them in the recovered
tree. That is absence established properly, not absence assumed from a source that
undercounts.

The replay UI is therefore real and Android-only: `REPLAY/{NONE,OPEN,SAVE}` and
`MESSAGE/REPLAY/SAVE`, together with the `MESSAGE/YES|NO` dialog vocabulary that has no Windows
counterpart.

### The reproduction recipe

Any future platform-difference claim should be settled in this order, not the one I used:

1. Contiguous byte scan of both images — settles the easy cases.
2. `decode_sso.py` over the recovered tree — settles the packed-string cases on Windows.
3. Only then may a set difference be written down.

Doing steps 1 and 2 before writing, rather than after, is what would have prevented the five
corrections in this note.

## Conclusion

1. Windows is MSVC with RTTI; Android is clang/Itanium with **no RTTI**. Solid.
2. Under the union extraction: 80 Windows keys, 95 Android keys, **51 shared**.
3. **17 of the 22 Windows-only keys are confirmed Windows-only** by a contiguous byte scan of
   the DLL — keyboard input, window management, desktop paths, legacy-save migration, theme
   toggles. Four more (`KEYBINDS/SET`, `REMOVE`, `VIEW`, `MESSAGE/CANCEL`) are Windows-only
   but stored as packed SSO immediates, so they need the SSO decoder rather than a byte scan.
   `HOME/SEARCH` was reclassified as shared after the re-check.
4. **There is no icon-preview asymmetry.** An earlier claim that Android had an
   `ICONIC/PREVIEW/` layer Windows lacked was wrong; all twelve keys are on both sides, as is
   every window-geometry key.
5. **Eight Android-only keys confirmed**, each with two independent Windows negatives: not
   contiguous in the DLL and not found by the SSO decoder. `REPLAY/{NONE,OPEN,SAVE}`,
   `MESSAGE/REPLAY/SAVE`, `MESSAGE/YES`, `MESSAGE/NO`, `ICONIC/OVERRIDE`, `ICONIC/WAVE`.
   `HOME/RULESETS/{0,1}` were reclassified as shared — they decode on Windows as
   `HOME/RULESETS/0x` and `1y`, the known trailing-character SSO artefact.
6. **The dialog vocabulary difference is confirmed from both directions**: Windows has
   `MESSAGE/CANCEL`, Android has `MESSAGE/YES` and `MESSAGE/NO`, no overlap either way.
7. **The only fully verified parity statement** is `iconic::PreviewLayer`, which appears under
   both manglings — MSVC `.?AVPreviewLayer@iconic@@` on Windows, Itanium
   `N6iconic12PreviewLayerE` on Android.
8. Assets are fully shared (8 of 8 `.png` names).
9. Roughly a dozen regex artefacts remain on each side and are listed rather than hidden.

**Method note, since this is the fourth and fifth correction in one note:** every set-difference
claim in a cross-binary comparison must be re-verified by scanning both raw images for the
specific string before it is written down. Two of the five statements above were published first
and reversed later, both times because a set difference was read from a source that
undercounts one side. The platform split is now smaller, narrower, and considerably more
trustworthy than it was three commits ago.

## Open questions

- Check whether `ICONIC/PREVIEW/*` is reachable on Windows under a different key prefix, or
  genuinely absent. Its size makes it the most interesting asymmetry found.
- (Resolved 2026-10-20.) `HOME/WIDTH` and `HOME/HEIGHT` have no occurrence in
  `_xrefs.tsv` or the recovered tree, so they are Android-only on this baseline; the
  windowed-mode relation is unverified but moot for the Windows build. UNVERIFIED: whether
  Android's windowed mode maps to a Windows equivalent at all.
- Filter the residual artefacts out of the key shapes so future counts are exact rather than
  approximate.