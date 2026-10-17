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

## Conclusion

1. Windows is MSVC with RTTI; Android is clang/Itanium with **no RTTI**.
2. The MSVC class inventory does not transfer to Android, and cannot be made to.
3. Settings keys transfer almost completely: every `ICONIC/` key and 28 of 46 `HOME/` keys are
   shared. Android's `ICONIC/` set is a strict superset of Windows'.
4. **Two prefixes are Windows-only: `KEYBINDS/` (5 keys) and `DISPLAY/` (2).** Both are input
   and window concerns.
5. The normalised class-name overlap remains open.

## Open questions

- Normalise the two ABIs' class encodings and produce a real overlap table.
- Determine whether Android carries `KEYBINDS/` and `DISPLAY/` under a different prefix.
- Check whether the 3 Android-only `HOME/` keys are new features or renamed ones.