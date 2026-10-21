# M1 residuals — pending inputs and plan

- **Date** — 2026-10-22.
- **Status** — M1 is declared `done` on the binary side (`docs/ROADMAP.md`). The items
  below are the residuals that cannot be closed from `_original.dll` alone. They are
  documented, not faked, and are the only things standing between M1 and "fully complete".
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`
  (SHA-256 `1635c339…`). The inputs below are *reference* material, never committed.

## User-provided inputs (stored, not yet analysed)

### 1. `hackpro.json` — settings registry contents

- **Source** — the user's installed Mega Hack folder (the mod directory).
- **Local copy** — `.oc/hackpro.json` (gitignored, 13,258 bytes). Original upload path:
  `C:\Users\Robert_\AppData\Local\Temp\opencode\uploads\d8fc9d92-3e94-4cac-9950-a2051a65f28f\hackpro.json`.
- **What it closes** — the settings-registry *contents* criterion (every key, its type,
  default, and the applying function). The mechanism is already mapped (184 tags, six
  accessors, container at `registry + 0x180`).
- **Analysis rule** — document keys/types/defaults and the accessor that applies each;
  cross-check against the 184 tags in `_settings.tsv`. **Do not commit the file or its
  values** — record schema and key names only. The user has explicitly consented to this
  one analysis; they have asked not to be asked for this kind of data for every hack.
- **Status** — received, not yet analysed. User said "don't start yet".

### 2. `GeometryDash.exe` — Geometry Dash 2.2081

- **Path** — `C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\GeometryDash.exe`
- **Version** — 2.2081, verified by the user.
- **Consent** — the user has explicitly approved opening it as a **read-only** analysis
  target. It must never be committed, redistributed, or treated as ground truth for
  anything other than the GD-side facts below.
- **What it closes** — the native Best Run field mapping (`(level, startPercent) ->
  bestEndPercent`) and the question of whether `+0x2C4`/`+0x35C` are `GJGameLevel`
  fields (they are UNVERIFIED from Mega Hack's binary; a machine-code sweep shows they
  live on a different GD structure).
- **Caveat** — this is a deliberate re-baselining against a second binary, which
  AGENTS.md requires to be done with a diffed offset map, not a silent mix. The result
  goes into `notes/gd-structures.md` as a clearly labelled GD-side section, distinct
  from the MH-verified offsets.
- **Status** — path recorded, not yet opened.

## Residuals I can close without user input

These are binary work against `_original.dll` only:

1. **Lifecycle** — TLS callbacks, `early-load` ordering, shutdown path (destructor
   wrapper `0x180155155` already flagged).
2. **Hook-target RVAs** — the 30 name-resolved sites in `_hook_sites.tsv` still need
   checking against thunks.
3. **Serialiser `this` base** in `notes/settings-registry.md` (UNVERIFIED).
4. **Exact C++ type of the registry's map container** (not `std::unordered_map`).
5. **Address map under version control** (cross-cutting).
6. **`samples/.../tools/` triage** (cross-cutting).

## User's second batch of inputs (2026-10-22)

### Additional files received (stored in `.oc/`, gitignored)

| File | Size | Notes |
| ---- | ---: | ----- |
| `hackpro.json` | 13,258 B | second upload; same size as the first. User states there is **no separate "default"** — these values are the ones to treat as defaults. |
| `home.json` | 587 B | mod data file from the v9 folder |
| `iconic.json` | 13,727 B | mod data file from the v9 folder |
| `keybinds.json` | 14,309 B | mod data file from the v9 folder |
| `best_runs.bin` | 49,352 B | the **patch's** Best Run sidecar (not the release's format) |
| `best_runs.bin.bak` | 49,352 B | backup of the above |

All six are user data: document schema/key names only, never commit values.

### Octavus path — VERIFIED (not a hallucination)

The audit's `%LOCALAPPDATA%\Octavus\` claim is **confirmed correct**, not a hallucination.
Disassembly of `FUN_180042FB0` (VA `0x180042FB0`) contains the string table at
`0x18053FBA5`: `Octavus\0data.json\0save.json\0...`. The GUID at `0x1805820E0` is
`FOLDERID_LocalAppData` (f1b32785-6fba-4fcf-9d55-7b8e7f157091), so the path resolves to
`%LOCALAPPDATA%\Octavus\{data,save}.json`.

The user could not find the folder because **it is created at runtime** — only when the
mod performs a save operation. The mod's own folder (`GeometryDash\geode\mods\...\v9\`)
holds the *mod's* config (`hackpro.json`, `home.json`, `iconic.json`, `keybinds.json`,
`best_runs.bin`); `Octavus` is a *separate* store used by the release for `data.json` /
`save.json` (GD-side state). The two coexist.

**Quest (optional, only for the save/load JSON schema):** play one level in Geometry Dash
with Mega Hack installed, then check whether `%LOCALAPPDATA%\Octavus\` appears. If it does,
share `data.json` / `save.json` (keys+types only is fine). If it does not appear after a
save, that is itself a finding (the path may be conditional on a setting).

## Binary-only closures (2026-10-22)

Closed without user input:
- Octavus path — verified (string table at 0x18053FBA5: Octavus\0data.json\0save.json).
  The folder is created at runtime on save; the user can trigger it by playing a level.
- Shutdown path — release-side shutdown is atexit(FUN_180001370) (resets CoreDirector
  vtables at 0x1808004D0/0x180800840/0x180800848, frees two allocations). TLS
  callbacks: none (PE TLS callback array at 0x18063F958 is empty).
- Lifecycle — early-load ordering mapped (tab-registration); TLS none; shutdown mapped.

Still open (deep, unlikely to close without runtime):
- Hook-target RVAs (30 name-resolved sites, not individually checked against thunks).
- Serialiser this base (UNVERIFIED).
- Exact C++ type of the registry's map container (UNVERIFIED).
## User's stated preference

The user can help analyse user-data parts (their own config, their own GD install) when
needed, but does not want to be asked for this kind of help for every hack. So: ask only
when a residual genuinely requires it, and batch the asks. The two items above are the
only ones that require user input; everything else is mine to close.

## Suggested order (not started)

1. Close the six binary-only residuals (items 1–6 above).
2. Analyse `.oc/hackpro.json` → close settings-registry contents criterion.
3. Open GD 2.2081 read-only → close Best Run + `+0x2C4`/`+0x35C`.
4. Final M1 audit pass, then declare M1 fully done and draft the M2 prompt.

The user has said "don't start yet" — this note exists so the inputs and the plan survive
the wait.
