# The host-method surface Mega Hack resolves at runtime

- **Question** — Mega Hack imports zero GD gameplay symbols, so what does it actually
  reach, and which of the patch's hook-site labels are corroborated by the release?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-18.
- **Tool** — `scripts/host_methods.py` → `recovered/functions/_host_methods.tsv`.

## Why this is the right unit

`notes/host-abi.md` records that of 1,010 imports over 32 DLLs, **zero** are GD gameplay
classes. Nothing is statically linked against `GJGameLevel`, `PlayLayer` or anything else;
every one of them is found by **string name at runtime**. The consequence for research is
that the *string* is the only handle on the host ABI — you cannot grep the import table,
because there is nothing there.

`_strings.tsv`'s `host-method` category is that handle, 214 methods. This script joins it to
`_xrefs.tsv` so each method carries its referencing functions.

```
distinct methods            : 214
functions naming them       : 350
named by more than 1 function: 58
```

## The inventory

| VA | Functions | Method |
| -- | --------: | ------ |
| `0x180545515` | 25 | `PlayLayer::resetLevel` |
| `0x18054552B` | 11 | `PlayLayer::postUpdate` |
| `0x1805454E8` | 10 | `PlayLayer::init` |
| `0x180575973` | 8 | `PlayLayer::destroyPlayer` |
| `0x180575EC3` | 6 | `GJBaseGameLayer::update` |
| `0x1805757D9` | 5 | `GJBaseGameLayer::collisionCheckObjects` |
| `0x18057508A` | 5 | `PlayLayer::addObject` |
| `0x1805454F8` | 5 | `PlayLayer::setupHasCompleted` |
| `0x1805749A4` | 4 | `LevelEditorLayer::init` |
| `0x18054555E` | 4 | `LevelEditorLayer::onPlaytest` |

The shape of the top of that list is itself the finding: Mega Hack's deepest coupling is to
the **level lifecycle**, not to rendering. `resetLevel`, `postUpdate` and `init` together
account for 46 of 350 references.

Note that four entries are not GD at all — `hackpro::Bool`, `hackpro::Decimal`,
`hackpro::Int`, `hackpro::String` (plus `hackpro::Bytes`) are Mega Hack's **own value
types**, named through the same resolver. They appear here because `classify_strings.py`
recognises them as `Class::method` shapes. That is worth knowing: the resolver is generic,
and the category is not a claim that the method is GD's.

This was later confirmed rather than assumed. `FUN_18002EE40` — the accessor for
`hackpro::Bytes` — turns out to emit that literal on a miss, which is how it was identified
among the accessors at all. See
[settings-registry.md](settings-registry.md#corrected-fun_18002ee40-is-getconfigvarhackprobytes-not-a-second-registry).

## Corroborating two of the patch's hook-site labels

The README lists six addresses whose role descriptions come from the local patch's notes
rather than an independent derivation. Two of them can now be corroborated from the release
binary alone, because Mega Hack resolves those very methods by name in the functions
adjacent to those addresses.

`FUN_1801551F0` — VA `0x1801551F0`, RVA `0x1551F0`, 213 bytes, one caller — names **all
three** of:

| String VA | Method |
| --------- | ------ |
| `0x180545515` | `PlayLayer::resetLevel` |
| `0x18054552B` | `PlayLayer::postUpdate` |
| `0x1805454E8` | `PlayLayer::init` |

And the patch's hook sites sit immediately after it:

| Patch label | Address | Inside |
| ----------- | ------- | ------ |
| Original init call through the load/key wrapper | `0x1801552D6` | after `FUN_1801551F0`, which ends at `0x1801552C5` |
| Reset / save wrapper | `0x180155408` | a later function |
| Post-update tracking wrapper | `0x18015546B` | a later function |

Two independent angles support this:

1. a **string xref** — `PlayLayer::resetLevel` and `PlayLayer::postUpdate` are named in
   Mega Hack's own code, in the function immediately preceding the hook region; and
2. a **call-graph path** — `FUN_1801551F0` has exactly one caller, so it is a registration
   or setup routine, which is what a function that resolves three lifecycle methods and
   hands them onward would be.

So "reset/save wrapper" and "post-update tracking wrapper" are no longer only the patch
author's labels: Mega Hack independently resolves the matching GD methods there. The other
four labels stay uncorroborated, and `0x180155155` / `0x1801553EC` in particular have no
resolver string pointing at them.

## Why Best Run storage is still not answered

The inventory contains the whole save path, which is where the search should start:

| VA | Method | Referrers |
| -- | ------ | --------- |
| `0x18057AEFC` | `GJGameLevel::savePercentage` | `FUN_180199D10` |
| `0x1805451DE` | `AppDelegate::trySaveGame` | `FUN_1800A8020`, `FUN_180151B40` |
| `0x18057615C` | `PlayLayer::storeCheckpoint` | `FUN_180176250` |
| `0x180545541` | `PlayLayer::updateProgressbar` | `FUN_1800A83B0`, `FUN_180165040` |
| `0x180574DCF` | `PlayLayer::updateAttempts` | `FUN_180158EB0` |
| `0x1805759E9` | `PlayLayer::showNewBest` | `FUN_180169DE0` |
| `0x180575E9F` | `GJBaseGameLayer::loadStartPosObject` | `FUN_180172B90` |

`GJGameLevel::savePercentage` is resolved in exactly **one** Mega Hack function,
`FUN_180199D10`. That is the place to read next: if Best Run rides on GD's save system
rather than on Mega Hack's own `hackpro.json`, the write has to go through that function.

UNVERIFIED: whether Best Run data is stored in GD's `data.json` or in `hackpro.json`.
The reasoning that pushes toward `data.json` is that `hackpro.json` is a flat
name → typed-value map (bool / int / double / string / bytes), which has no room for a
per-level `(level, startPercent) -> bestEndPercent` table. That is an argument from the
schema's shape, not a derivation, and `FUN_180199D10` has not been read yet.

## Open questions

- (Resolved 2026-10-20.) Best Run is not stored by Mega Hack: the full hook surface (338
  sites) contains only `GJGameLevel::savePercentage`, which can veto but not write. See
  notes/persistence.md. `FUN_180199D10` registers the three hooks (`savePercentage`,
  `levelComplete`, `resetLevel`) and contains no storage code.
- `FUN_180042FB0`'s legacy-save path is reachable from a UI key
  (`HOME/MISC/LOAD_LEGACY_SAVE`, `ERROR/HOME/LEGACY_SAVE_PARSE`,
  `ERROR/HOME/LEGACY_SAVE_KEYBINDS`), which implies Mega Hack can import an **older
  Mega Hack save layout**. Whether that includes Best Run is the same question as above.
- Whether any of the 214 methods is resolved through Geode rather than through Mega Hack's
  own resolver. `classify_strings.py`'s category does not distinguish the two.

## Failure modes recorded

- **`refs` vs referring functions.** `_xrefs.tsv` keeps one row per string, so its `refs`
  column counts *strings*. Copying it into a per-method table understates the surface:
  `AppDelegate::trySaveGame` has one row and two referrers. The first version of
  `host_methods.py` did exactly that and reported every one of the 214 methods as having
  exactly one referrer. `functions` is a comma-joined cell, so the recount has to split it.
- **`#`-prefixed headers.** `_xrefs.tsv` writes its header as `# string_va\t…`. A plain
  `csv.DictReader` treats the first *data* row as the header and every column lookup
  raises `KeyError`. `read_tsv` in `host_methods.py` handles this; any new script reading
  these indexes needs the same.
- **A query must not write the index it queried.** The first version wrote whatever
  survived `--grep` to `_host_methods.tsv`, so `host_methods.py --grep zzz` silently
  truncated the index to zero rows and printed no error — the next reader would have got an
  empty inventory that looked like a result. `--grep` now prints and leaves the file alone.
  Same class of bug as the wrong-`refs` one above: the first output looked fine, and the
  damage was in a file nobody was looking at.