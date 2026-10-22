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

## Validation status

UNVERIFIED / blocked: no matching Geode 5.6.1 SDK is available in this
environment. The only installed SDK source is Geode 5.11.1
(`C:\Users\Robert_\Documents\Geode`), while the installed loader and the
baseline are 5.6.1. Building against 5.11.1 would not be a valid test of the
baseline skeleton, and running a mismatched mod under the 5.6.1 loader is
not safe.

The reconstruction's compile-time stand-ins (`geode::Mod`, `geode::Loader`,
`geode::async::Runtime`, and the `geodeImplicitEntry` stub) exercise the
control flow but do not constitute a loader-validated skeleton.

## Conclusion

The skeleton is specified by the recovered entry path and metadata, but it is
not loader-validated in this repository. Marking the M2 exit criterion
complete would require a Geode 5.6.1 SDK and a runnable Geometry Dash 2.2081
session, neither of which is available here.
