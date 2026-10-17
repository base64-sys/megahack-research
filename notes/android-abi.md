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
3. Android's method surface *is* recoverable from `_ZN` symbols, and is the practical route.
4. The normalised class-name overlap remains open.

## Open questions

- Normalise the two encodings and produce a real overlap table.
- Re-run the string inventory (`_xrefs.tsv`) against an Android binary to check whether
  setting keys and format strings are shared, which may be a better parity check than class
  names.
- Determine whether Android is `-fno-rtti` by policy or by build flag, since that decides
  whether M5 can ever recover its class set.