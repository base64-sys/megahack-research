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
