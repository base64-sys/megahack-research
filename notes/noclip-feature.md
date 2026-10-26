# NoClip feature cluster

- **Question** — which code and settings implement Mega Hack's NoClip features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

| Setting key | Observed value | Role |
| --- | --- | --- |
| `LEVEL/NOCLIP/DROPDOWN/PLAYER_1` | True | Enable NoClip for player 1 |
| `LEVEL/NOCLIP/DROPDOWN/PLAYER_2` | True | Enable NoClip for player 2 |
| `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE` | True | Noclip safe-mode/disable-progress option (tag binding UNVERIFIED) |
| `LEVEL/NOCLIP/DROPDOWN/TINT/CHECKBOX` | True | Tint overlay |
| `LEVEL/NOCLIP/DROPDOWN/TINT/COLOUR` | 2303 | Tint colour |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/ACCURACY_LIMIT_ON` | False | Accuracy limit |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/DEATH_LIMIT_ON` | True | Death limit |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/ACCURACY_LIMIT` | 99.0 | Accuracy limit value |
| `LEVEL/NOCLIP_LIMITS/DROPDOWN/DEATHS_LIMIT` | 11 | Death limit value |
| `LEVEL/NO_COLLISION/DROPDOWN/HAZARDS` | True | Disable hazard collision |
| `LEVEL/NO_COLLISION/DROPDOWN/SLOPES` | True | Disable slope collision |
| `LEVEL/NO_COLLISION/DROPDOWN/SOLIDS` | True | Disable solid collision |
| `LEVEL/NO_COLLISION/DROPDOWN/SPECIALS` | True | Disable special collision |
| `STATUS/NOCLIP_ACCURACY/DROPDOWN/PREFIX` | False | Status label prefix |
| `STATUS/NOCLIP_DEATHS/DROPDOWN/UNITS` | True | Status label units |

## Evidence — implementation

- `core::NoclipTracker` is constructed by `FUN_180201300`
  (`0x180201300`).
- `NoclipTint` is constructed by `FUN_18017A500`
  (`0x18017A500`), which installs multiple vtables.
- `HACK_NOCLIP_LIMITS` and `CHECKBOX_NOCLIP_*` strings are built in
  `FUN_180062940` (`0x180062940`), the UI builder.
- `Noclip: {:.2f}%` is formatted in `FUN_180152010`
  (`0x180152010`).

## Evidence — Disable Progress gate

- `CHECKBOX_NOCLIP_DISABLE_PROGRESS` = tag 73 (`0x49`), Map A. Its entry is inserted at
  store VA `0x180064D8D` in the 21-entry static map; see
  [static-hack-label-map.md](static-hack-label-map.md).
- `FUN_18019B5F0` (`0x18019B5F0`) reads it through `FUN_18002DE50` and returns true when
  the tag is set and `FUN_180199520() < 1.0` — when some level time was attributed to a
  cheat frame (`DAT_18080CB68 == 1`).
- The effect is progress-side: `FUN_180199DF0` (the `GJGameLevel::savePercentage` hook)
  returns without calling the original, and `FUN_180199E30` (the `PlayLayer::levelComplete`
  hook) runs the original with `PlayLayer+0x3230` set. See
  [safe-mode-feature.md](safe-mode-feature.md).
- UNVERIFIED: which JSON key writes tag 73. `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE` is the only
  observed Noclip safety key and is `True` in the sampled profile, but no recovered code
  binds that path to 73.

## Open questions

- UNVERIFIED: which functions read each NoClip setting tag and apply the
  collision bypass.

## Evidence — Noclip Limits (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18017A290` (`0x18017A290`) also implements the Noclip Limits feature
(when `HACK_NOCLIP_LIMITS` = tag 83 is enabled):

1. **Accuracy limit** (tag 0x55 = 85, `CHECKBOX_NOCLIP_ACCURACY_LIMIT`):
   if enabled, reads tag 0x54 (84, a double) as the accuracy threshold. Computes
   `FUN_180199560()` (current noclip accuracy) and sets `bVar12 = (accuracy <= threshold)`.

2. **Death limit** (tag 0x57 = 87, `CHECKBOX_NOCLIP_DEATH_LIMIT`):
   if enabled, reads tag 0x56 (86, a uint) as the death count threshold. If it
   matches `FUN_1801994E0()` (current death count), sets
   `bVar12 |= (FUN_1801994D0() ^ 1)` (cheat flag from previous frame).

3. If `bVar12 != 1` (limits not exceeded): skips the tint overlay (goto
   `LAB_18017a37d`).

### Tag map (limits)

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x53 | 83 | `isHackEnabled` (Map B) | `HACK_NOCLIP_LIMITS` | Master toggle |
| 0x55 | 85 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_ACCURACY_LIMIT` | Accuracy limit toggle |
| 0x54 | 84 | `FUN_180003D80` (double accessor) | (not in static list) | Accuracy threshold value |
| 0x57 | 87 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_DEATH_LIMIT` | Death limit toggle |
| 0x56 | 86 | `FUN_18002E4C0` (uint/int accessor) | (not in static list) | Death count threshold

UNVERIFIED: which JSON keys bind to tags 0x54 and 0x56; the exact semantics of
`FUN_180199560` (current accuracy) and `FUN_1801994E0` (death count).
- UNVERIFIED: whether `NoclipTint` is drawn through the render hook cluster or
  a separate path.
- UNVERIFIED: the exact meaning of the accuracy and death limits.
- UNVERIFIED: the JSON key→tag binding for tag 73 (candidate:
  `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE`) and for the Safe Mode flags 481/482.

## Evidence — Noclip Tint (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18017A220` (`0x18017A220`) registers `FUN_18017A290` (`0x18017A290`)
as a hook on `PlayLayer::destroyPlayer` (RVA `0x3B39D0`).

`FUN_18017A290` handles the tint overlay:
- If `isHackEnabled(HACK_NOCLIP_LIMITS = 83, 0x53)` is false: skips the
  limits check.
- Reads `CHECKBOX_NOCLIP_PLAYER_1` (tag 77) and `CHECKBOX_NOCLIP_PLAYER_2`
  (tag 78) to determine which player triggered the collision.
- If `CHECKBOX_NOCLIP_TINT` (tag 75) is true: applies a tint overlay using
  `DAT_18080C710` (a CCNode) with fade/hide animations. The tint colour
  comes from tag 0x4C (76, a spinner value).
- When players differ and player 2 is enabled (`PlayLayer+0x422 == 1`), the
  tint is applied to the other player's node.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x53 | 83 | `isHackEnabled` (Map B) | `HACK_NOCLIP_LIMITS` | NoClip limits master toggle |
| 0x4D | 77 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_PLAYER_1` | Player 1 tint toggle |
| 0x4E | 78 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_PLAYER_2` | Player 2 tint toggle |
| 0x4B | 75 | `getConfigVar<bool>` (Map A) | `CHECKBOX_NOCLIP_TINT` | Tint overlay toggle |
| 0x4C | 76 | `FUN_18002E4C0` (uint/int accessor) | (not in static list — runtime-assigned) | Tint colour value

UNVERIFIED: which JSON keys bind to tag 0x4C; the exact tint colour encoding;
the semantics of `DAT_18080C710` and the animation parameters.

## Evidence — Noclip Accuracy display (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180151FC0` (`0x180151FC0`) registers `FUN_180152010` (`0x180152010`)
as a hook on `EndLevelLayer::showLayer` (RVA `0x134CA0`).

`FUN_180152010` draws the "Noclip: {:.2f}%" label:
- If `isHackEnabled(tag 0x1F1 = 497)` is true: returns immediately (hides the
  accuracy display).
- If false AND there's a current level: draws the accuracy label with
  checkpoint/position data.

The function also reads `HACK_NOCLIP` (tag 72) and uses `FUN_18008A510` for
settings access. It formats "Noclip: {:.2f}%" using `FUN_1803A50B4` for
string allocation.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1F1 | 497 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | "Hide Noclip Accuracy" toggle (inverted) |
| 0x48 | 72 | `isHackEnabled` (Map B) | `HACK_NOCLIP` | Master NoClip toggle (read for context)

UNVERIFIED: which JSON key binds to tag 0x1F1; whether this is the "Noclip
Accuracy" feature or a separate "hide accuracy" toggle; the exact rendering path
for the label.

## Evidence — NoClip for P1/P2 and the limit gate in the destroyPlayer detour (2026-10-26)

One detour carries every NoClip-adjacent setting: tag 72 installs it on
`PlayLayer::destroyPlayer`, and the body then reads the limit, per-player and tint tags at
runtime. That is why the P1/P2 option has no registration of its own.

| Field | Value |
| --- | --- |
| Installing tag | 72 (`0x048`, `HACK_NOCLIP`, static map entry 33, store `0x1800635D3`) |
| Registration | `0x1800B2723` (`notes/hook-registrations.md`) |
| Callback | `FUN_18017A220` (body `0x18017A220..0x18017A278`, `ret` at `0x18017A278`) |
| Disable branch | `0x18017A279..0x18017A289`, tail-jumps `0x180108A80` |
| Target | `PlayLayer::destroyPlayer`, RVA `0x3B39D0` (resolved via `0x18010A370` from base `0x18010A250`) |
| Name string | `0x180575973` "PlayLayer::destroyPlayer" |
| Detour | `0x18017A290..0x18017A4F0` (0x260 bytes, code cave), tail `jmp 0x1801A3D20` at `0x18017A403` |
| Installer | `0x180169480`, args `[rsp+0x20]=0xFFFFFBF8`, `[rsp+0x28]=1` |

The detour is a `/GS`-protected body (cookie `0x1807F7040`, failure path `0x18017A4F1` calling
`0x1803A52E0`) and it returns through the same delayed-bind trampoline the Jumpscare detour
uses for this target, so the original `destroyPlayer` still runs.

### Instruction-level flow

| Site | Read | Effect |
| --- | --- | --- |
| `0x18017A2B7` | `[layer+0x3788]` | object filter; equal -> straight to the tail jump |
| `0x18017A2CB` | tag 83 (`0x53`, `HACK_NOCLIP_LIMITS`) via `0x18008F420` | master for the two limits |
| `0x18017A2E0` | tag 85 (`0x55`, `CHECKBOX_NOCLIP_ACCURACY_LIMIT`) bool | when set, read double tag 84 (`0x54`) and compare with `0x180199560()`; `ucomisd` + `setae bpl` |
| `0x18017A330` | tag 87 (`0x57`, `CHECKBOX_NOCLIP_DEATH_LIMIT`) bool | when set, read int tag 86 (`0x56`), `dec`, compare with `0x1801994E0()`; on equality `0x1801994D0() ^ 1` is OR-ed into `bpl` |
| `0x18017A37D` | tag 77 (`0x4D`, `CHECKBOX_NOCLIP_PLAYER_1`) bool | saved in `bpl` |
| `0x18017A38C` | tag 78 (`0x4E`, `CHECKBOX_NOCLIP_PLAYER_2`) bool | saved in `al`; `cmp bpl,al; je 0x18017A408` — equal flags skip the per-player filter |
| `0x18017A39E` | `byte [layer+0x422]` | must be 1 (two-player level) for the filter to apply |
| `0x18017A3AC` | `[layer+0xDA0]` | player 1 object; the destroyed object (`rsi`, second argument) must match it when only P1 is enabled |
| `0x18017A3C8` | `[layer+0xDA8]` | player 2 object, same test for P2 |
| `0x18017A3D1` | — | return path: no tint |
| `0x18017A408` | tag 75 (`0x4B`, `CHECKBOX_NOCLIP_TINT`) bool | tint block |
| `0x18017A41D` | `[0x18080C710]` | `NoclipTint` instance (RTTI `.?AVNoclipTint@@` at `0x1807F7E10`); null -> skip |
| `0x18017A42B` | int tag 76 (`0x4C`) colour | decoded by `0x1801F15E0` into three bytes, reordered, written through `[tint+0x198]` slot 0 |
| `0x18017A488` | slot `[tint+0x198]+0x28` with `edx=0x40` | opacity 64 |
| `0x18017A495` | slot `[tint]+0x140` with `edx=1` | the shared show/hide slot used by Hide UI and Hide Pause Button/Menu |
| `0x18017A49B` | `CCHide::create` + `CCFadeTo::create(0.15f, 0)` via `0x1800BADB0` + `runAction` | fade the tint out (constant `0x180577384` = `0x3E19999A`, 0.15f) |

So the P1/P2 pair is read in the tint path and decides whether the destroyed player is the
one whose NoClip is enabled; the limit pair decides whether the code reaches the tint block at
all. Both are runtime tags inside a detour installed by the master NoClip toggle, which is why
mapping them needs the body rather than a registration row.

Corroboration: every tag above already has a static label (map entries 8-13, 33, 34), and the
mapped "Noclip Limits" row independently names tag `0x55`/`0x54` (accuracy) and `0x57`/`0x56`
(death) as gating the tint overlay. This body is the site where those reads happen, so the two
records agree rather than one being inferred from the other.

### The death counter label

The v7.0 wording also promises a death *counter*. The image contains `"Deaths: {}"` at
`0x180574AAB`, referenced exactly once (`0x180152A4B`), inside a label builder that calls
`0x1801994E0()` at `0x180152A00` (the same death-count getter the death-limit comparison
uses), builds the object key `"deaths-label"` from the immediates `0x6C2D736874616564`
(`"deaths-l"`) and `0x6C656261` (`"abel"`), formats through `0x18000AE30`, and creates a
`CCLabelBMFont` with `"goldFont.fnt"` (`0x180572B9F`) scaled from `0x180545688`.

UNVERIFIED: which tag gates that deaths label, so the v7.0 "Noclip Death Counter & Death
Limit" row stays `cluster`; the JSON keys that write tags 77/78/84/86; the exact meaning of
`byte [layer+0x422]` (read here as the two-player flag, consistent with the P2 hitbox draw) and
of `[layer+0x3788]`; and the direction of the limit predicates — the code skips the tint block
when the accumulated flag is 1, but "within limit" versus "limit exceeded" is not settled by
the instructions alone.
