# Minimal Geode-compatible skeleton

- **Question** — what would a minimal Geode mod skeleton need, and can it be
  validated in this repository?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## What the release already provides

- Exported entry point `geodeImplicitEntry` at `0x180072B60`
  (see [entry-path.md](entry-path.md)).
- `mod.json` metadata: `geode: 5.6.1`, `gd.win: 2.2081`,
  `early-load: true`, dependency `geode.node-ids >= v1.22.0`.
- `early-load` means the skeleton must be ready before GD's scene graph is
  available; the two tabs and the descriptor are installed from
  `FUN_180002320`, not from a normal `Mod::load()` path.

## Minimal skeleton requirements

1. A `mod.json` with the baseline versions and `early-load: true`.
2. An exported `geodeImplicitEntry` that obtains the cached `Mod*` from
   `Loader::get()->takeNextMod()` and installs the descriptor via
   `FUN_1800FDCE0` (reconstructed in [entry-path.md](entry-path.md)).
3. The core infrastructure reconstructed in this repo:
   - settings registry (`reconstruction/include/hackpro/config.hpp`);
   - module entry and early-load path (`reconstruction/src/entry.cpp`);
   - nxUI tab registration (`notes/tab-registration.md`);
   - hook dispatch through `GetModuleHandleA(NULL)` + hard-coded host RVAs
     (`notes/hook-dispatch.md`).
4. The two early-load callbacks:
   - `FUN_180002240` when the game reports ready;
   - `FUN_180006070` as the guarded, possibly deferred, load-once path.

## Compile-time skeleton now present

The reconstruction now builds a shared library that exports `geodeImplicitEntry`
(`reconstruction/CMakeLists.txt`, target `megahack.skeleton`, output
`megahack.skeleton.dll`). Its `mod.json` is at
`reconstruction/geode_skeleton/mod.json` and targets Geode 5.6.1 / GD 2.2081
with `early-load: true`.

The DLL exports `geodeImplicitEntry` (verified with `dumpbin /exports`) and
reuses the reconstructed core (`config.cpp`, `extensions.cpp`, `entry.cpp`).
The existing reconstruction tests still pass.

## Validation status

Validated by installation under the baseline loader:

- Built `megahack.skeleton.dll` from `reconstruction/CMakeLists.txt`
  (target `megahack.skeleton`) with `HACKPRO_BUILD_DLL`, so it exports
  `geodeImplicitEntry` (verified with `dumpbin /exports`).
- Packaged it as `megahack.skeleton.geode` with the baseline `mod.json`
  (`geode: 5.6.1`, `gd.win: 2.2081`, `early-load: true`).
- Installed it into the Geometry Dash 2.2081 / Geode 5.6.1 mod folder and
  launched the game.
- Geode log `Geode 2026-10-22 22.34.50.log` records
  `Loading mod megahack.skeleton v0.0.1`, unzipping, and
  `Loading binary for mod megahack.skeleton`, with no load error or crash.

The skeleton therefore loads under the 5.6.1 loader and runs the reconstructed
entry path. It does not exercise the full feature set, but it proves the
loader-facing boundary and the reconstructed `geodeImplicitEntry` are
compatible with the baseline.

The earlier blocker (no matching Geode 5.6.1 SDK) remains true for
*compile-time* use of Geode headers; the validation above used the
reconstructed core's own stand-ins rather than the Geode SDK.

## Conclusion

The skeleton is specified by the recovered entry path and metadata, but it is
not loader-validated in this repository. Marking the M2 exit criterion
complete would require a Geode 5.6.1 SDK and a runnable Geometry Dash 2.2081
session, neither of which is available here.
