# Android symbol parity (initial pass)

- **Question** — can Mega Hack's Android ARM64 build be mapped through its `_ZN`
  symbols the way the Windows build is mapped through RTTI?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Binaries** — `absolllute.megahack.android32.so`,
  `absolllute.megahack.android64.so`.

## Method

1. `readelf --wide --symbols` on the ARM64 `.so`.
2. Count `_ZN` symbols and tally their owning class prefixes.
3. Compare with the Windows RTTI inventory (497 classes) and the hook-method
   surface (214 named GD methods).

## Evidence

- The ARM64 build has **2,760 defined `_ZN` symbols**.
- Their owning prefixes are overwhelmingly dependencies, not Mega Hack classes:
  - `arc`: 169
  - `fmt`: 135
  - `asp`: 90
  - `std::__ndk1`: 13 (shown as `std23`)
  - `geode`: 2
- No `PlayerObject`, `PlayLayer`, `GJBaseGameLayer`, `hackpro`, or `MegaHack`
  class prefixes appear among the defined symbols.
- The Android build therefore does **not** expose Mega Hack's own method names
  through defined ELF symbols, even though it contains thousands of `_ZN`
  symbols from statically linked dependencies.

## Consequence for M5

The Windows mapping route (MSVC RTTI + named host methods) does not transfer.
Android parity must be driven by string keys and behaviour, not by class/method
names. The 24 shared `HOME/...` keys and the zero `KEYBINDS/...` keys noted in
[android-abi.md](android-abi.md) are the more useful anchors.

## Open questions

- UNVERIFIED: whether Mega Hack's own Android code is stripped of `_ZN` names or
  compiled with a different mangling/visibility setting.
- UNVERIFIED: whether the Windows host-method list (214 names) has any Android
  equivalent beyond the shared string keys.
- UNVERIFIED: whether `android32` and `android64` differ in any Mega
  Hack-specific symbol beyond the one-symbol count difference already noted.
