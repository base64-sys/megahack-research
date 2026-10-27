# Address map (Mega Hack v9.1.3, GD 2.2081)

- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`
  (SHA-256 `1635c339…`), base `0x180000000`.
- **Tool** — `scripts/pe_rva.py` is the only RVA→file-offset converter (tracked, tested).
- **Date** — 2026-10-22.

All VAs are in Mega Hack's image unless marked GD. RVAs (GD) are relative to
`GeometryDash.exe` base `0x140000000`.

## Mega Hack — entry / lifecycle

| VA | Symbol / role |
| --- | --- |
| `0x180072B60` | `geodeImplicitEntry` (root) |
| `0x180002320` | early-load setup (tab registration + config load) |
| `0x180155155` | shutdown/destructor wrapper (UNVERIFIED, patch-side) |

## Mega Hack — settings registry

| VA | Symbol / role |
| --- | --- |
| `0x180800F58` | registry global (`.data` zero-fill, ≥ `0x4A8` bytes) |
| `0x1800A27E0` | static constructor `FUN_1800A27E0(&DAT_180800F58)` |
| `0x18008A510` | `FUN_18008A510` (returns `registry + 0xD8` = Config) |
| `0x180072DF0` | `tagFor` (name → tag) |
| `0x180072E20` | `keyFor` (tag → name) |
| `0x18008B490` | by-tag setter |
| `0x18008B780` | config loader (reads the definition stream) |
| `0x18007C700` | intern (tag, name) |
| `0x18007CB60` | string-keyed lookup (name → tag), container at `registry + 0x130` |
| `0x18002EE40` | `getConfigVar<hackpro::Bytes>` (tag arg index 2) |
| `0x18008F420` | `isHackEnabled` / Map B hack-flag getter (tag arg index 1) |
| `0x18002DE50` | `getConfigVar<bool>` / Map A config getter (tag arg index 1) |
| `0x180003D80` | `getConfigVar<scalar>` (tag arg index 1) |
| `0x18002E4C0` | `getConfigVar<int>` (tag arg index 1) |
| `0x18002D710` | `getConfigVar<std::string>` (tag arg index 2) |

## Mega Hack — patch state block (writable, in image)

| VA | Role |
| --- | --- |
| `0x18081BFC0` | mutable state block (48 bytes) |

## GD runtime resolution (Mega Hack calls into GD via RVAs)

| RVA (GD) | Symbol |
| --- | --- |
| `0x3B8EB0` | `PlayLayer::resetLevel` |
| `0x3A7A80` | `PlayLayer::levelComplete` |
| `0x16C8B0` | `GJGameLevel::savePercentage` |

## Geometry Dash 2.2081 (read-only reference, base 0x140000000)

| VA | Symbol / role |
| --- | --- |
| `0x14016C8B0` | `GJGameLevel::savePercentage` (Best Run fields at `+0x308..+0x34C`, `+0x4A8`) |

## Host ABI

| VA | Symbol / role |
| --- | --- |
| `0x180800840` / `0x180800848` | stream descriptor for config load (zero-fill, runtime) |
| `0x1806FE004` | compressed buffer: HOME tab UI (`0x2D5` bytes, gzip) |
| `0x1806FE2D9` | compressed buffer: KEYBINDS tab UI (`0xC2` bytes, gzip) |
| `0x1806FDDA0` | compressed buffer: replay rules (`0x264` bytes, gzip) |
| `0x1806FE39B` | compressed buffer: translation dict (`0x4B0DE` bytes, gzip) |

## Verified GJGameLevel offsets (Mega Hack side)

`+0x148`, `+0x14C`, `+0x158`, `+0x198`, `+0x1B8` (each from two angles).
`+0x2C4`, `+0x35C` — UNVERIFIED as GJGameLevel offsets (live on a different GD
structure; see `notes/gd-structures.md`).

## Verified PlayLayer offset

`PlayLayer + 0x3230`.

## PE sections (Mega Hack)

Base `0x180000000`. RVA→file-offset via `scripts/pe_rva.py` only. The patch state
block at `0x18081BFC0` is past `VirtualSize` (gap, not file-backed).


## Tools triage (samples/absolllute.hackmega/tools/)

None of these are tracked. Decision: they stay in samples/ (untracked) because they
depend on the vendored .analysis_tools (pefile, capstone, ghidra_scripts). Promoting
them to scripts/ would require vendoring those dependencies, which is out of scope.

- General PE tools (pe_disasm, pe_exports, pe_imports, pe_xrefs, pe_caves,
  inspect_interface_map) — reused during research, run with PYTHONPATH=samples/.../.analysis_tools.
- Patch/lifecycle tools (best_runs_db, best_runs_editor, build_persistence_patch,
  validate_persistence_patch, update_geode_package, import_dt_setting.ps1) — specific to
  the persistence patch experiment, not part of the release reconstruction.
- ghidra_scripts/, __pycache__/ — vendored/generated, not original.

The tracked replacements are scripts/pe_rva.py (RVA->offset), scripts/classify_strings.py,
scripts/settings_index.py, scripts/rtti_index.py, scripts/init_table.py, scripts/host_methods.py,
scripts/hook_sites.py, scripts/make_progress_svg.py.
