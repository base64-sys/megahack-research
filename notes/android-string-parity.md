# Android string parity (initial pass)

- **Question** — which definition-key namespaces survive on Android, and where do
  the platforms diverge?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Binaries** — `absolllute.megahack.android32.so`,
  `absolllute.megahack.android64.so`.

## Method

1. Extract printable ASCII runs of length >= 4 from the ARM64 `.so`.
2. Count runs beginning with each Windows definition-key namespace.
3. Compare with the Windows inventory in
   [definition-keys.md](definition-keys.md).

## Evidence

| Namespace | Android64 runs | Windows notes |
| --- | ---: | --- |
| `HOME/` | 32 | 20 path keys |
| `ICONIC/` | 25 | 22 path keys |
| `MESSAGE/` | 13 | 13 path keys |
| `ERROR/` | 9 | 11 path keys |
| `STATUS/` | 0 | Used for Best Run labels |
| `KEYBINDS/` | 0 | 5 path keys |

## Consequence

- Core configuration and cosmetic namespaces are broadly shared.
- `KEYBINDS/` is absent on Android, matching the earlier finding that keybinds
  are a Windows-only feature.
- `STATUS/` is absent on Android, consistent with Best Run being a
  Windows/GD-side feature rather than a portable one.

## Open questions

- UNVERIFIED: whether Android drops these features entirely or routes them
  through different key prefixes.
- UNVERIFIED: whether the extra `HOME/` and `ICONIC/` runs are distinct keys or
  formatting artefacts.
- UNVERIFIED: whether the same namespaces survive in the 32-bit build.
