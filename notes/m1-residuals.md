# M1 residuals — inputs, closures, and what remains

- **Date** — 2026-10-22.
- **Status** — M1 is `done` in [docs/ROADMAP.md](../docs/ROADMAP.md). This note originally
  tracked pending inputs and a plan; all of it ran, and M1 closed the same day. It has been
  updated in place so nothing reads "pending" that is not: each closed item now points at
  the note that closed it, the input inventory and consent terms are preserved verbatim,
  and the residuals that remain open are listed at the end.
- **Ground truth** — `samples/absolllute.hackmega/absolllute.hackmega_original.dll`
  (SHA-256 `1635c339…`). The inputs below are *reference* material, never committed.

## User-provided inputs (stored; analysis completed)

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
- **Status** — **CLOSED 2026-10-22.** Analysed; 202 keys over 5 types with observed values
  are in [settings-contents.md](settings-contents.md). The file itself was never committed.

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
- **Status** — **CLOSED 2026-10-22 (read-only).** Results:
  - [gd-structures.md](gd-structures.md) — five of the seven offsets verified from two
    angles each on both images; `+0x2C4`/`+0x35C` shown to belong to a *different* GD
    structure (the object returned at RVA `0x17B4A0`), so they are UNVERIFIED as
    `GJGameLevel` offsets.
  - [persistence.md](persistence.md) — native Best Run fields are GD's schema:
    `GJGameLevel+0x308..+0x34C` and `+0x4A8`, read by `GJGameLevel::savePercentage`.
  - [m1-verification.md](m1-verification.md) (Phase 3) — image base `0x140000000`, PE
    timestamp `2026-01-21 01:36:32`; `savePercentage` at RVA `0x16C8B0` resolves to a
    prologue with the documented field accesses.
  - Caveat recorded there: no `2.2081` string exists in the image, so the version rests
    on the user's statement — UNVERIFIED from either binary.

## Residuals closed without user input (2026-10-22)

The original list of "residuals I can close without user input" ran as follows:

1. **Lifecycle** — **closed.** TLS callbacks: none (the PE TLS callback array at
   `0x18063F958` is empty); early-load ordering mapped in
   [tab-registration.md](tab-registration.md); release-side shutdown is
   atexit(`FUN_180001370`), which resets the CoreDirector vtables at `0x1808004D0` /
   `0x180800840` / `0x180800848` and frees two allocations. The patch-side wrapper at
   `0x180155155` remains UNVERIFIED on the release side.
2. **Hook-target RVAs** — **still open** (see below).
3. **Serialiser `this` base** — **still open** (see below).
4. **Exact C++ type of the registry's map container** — **still open** (see below).
5. **Address map under version control** — **closed**: [address-map.md](address-map.md);
   all 31 entries resolve (27 file-backed, 3 zero-fill, 1 gap — re-derived in
   [m1-verification.md](m1-verification.md), Phase 1, row 7).
6. **`samples/.../tools/` triage** — the originality triage is recorded in
   [address-map.md](address-map.md) ("Tools triage"): everything stays untracked in
   `samples/` because it depends on the vendored `.analysis_tools`; `scripts/` holds the
   tracked replacements. ROADMAP's cross-cutting row for this still reads `open` — treat
   the triage in `address-map.md` as the current decision until the tracker is updated.

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

## Still open (deliberately, and why)

These cannot be closed from `_original.dll` alone, or need runtime observation:

- **Hook-target RVAs** — the 30 name-resolved sites in `_hook_sites.tsv` are not
  individually checked against thunks; the rows stay flagged UNVERIFIED per row.
- **Serialiser `this` base** in [settings-registry.md](settings-registry.md) —
  disassembly admits two readings; needs a dynamic observation. UNVERIFIED.
- **Exact C++ type of the registry's map container** — not `std::unordered_map`
  (bucket stride 16); the precise type is not recoverable from the binary. UNVERIFIED.
- **Save/load JSON schemas** — user data, deliberately not committed. The optional quest
  below is the only route to them.
- **`+0x2C4` / `+0x35C` as `GJGameLevel` offsets** — closed *negatively*: the sweep shows
  they belong to a different GD structure. Listed here so the question is not reopened;
  the evidence is in [gd-structures.md](gd-structures.md).

### The optional quest (still available, only for the save/load JSON schema)

Play one level in Geometry Dash with Mega Hack installed, then check whether
`%LOCALAPPDATA%\Octavus\` appears. If it does, share `data.json` / `save.json` (keys+types
only is fine). If it does not appear after a save, that is itself a finding (the path may
be conditional on a setting).

## User's stated preference

The user can help analyse user-data parts (their own config, their own GD install) when
needed, but does not want to be asked for this kind of help for every hack. So: ask only
when a residual genuinely requires it, and batch the asks. The quest above is the only
remaining item that requires user input; everything else is agent-closable.

## What the original plan produced (historical)

The "Suggested order" written before M1 closed ran to completion on 2026-10-22:
binary-only residuals closed (this note, section above); `.oc/hackpro.json` analysed →
[settings-contents.md](settings-contents.md); GD 2.2081 opened read-only →
[gd-structures.md](gd-structures.md) and [persistence.md](persistence.md); final audit
pass → [m1-audit.md](m1-audit.md) and [m1-verification.md](m1-verification.md), with M1
declared complete in [docs/ROADMAP.md](../docs/ROADMAP.md).
