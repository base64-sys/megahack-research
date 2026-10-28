# Status labels feature cluster

- **Question** — which settings and hooks implement Mega Hack's status labels?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

- `STATUS/HIDE` — master hide toggle.
- `STATUS/COSMETIC/OPACITY`, `STATUS/COSMETIC/SCALE` — cosmetic scaling.
- Field positions: `STATUS/FIELDS/0..23/DROPDOWN/POSITION`.
- Field format: `STATUS/FIELDS/8/FORMAT` (observed `'{best_run}'`).
- Units: `STATUS/ATTEMPTS/DROPDOWN/UNITS`,
  `STATUS/CPS/DROPDOWN/UNITS`, `STATUS/FPS/DROPDOWN/UNITS`,
  `STATUS/FRAME/DROPDOWN/UNITS`, `STATUS/JUMPS/DROPDOWN/UNITS`,
  `STATUS/NOCLIP_DEATHS/DROPDOWN/UNITS`.
- Toggles: `STATUS/CPS/DROPDOWN/MAX`, `STATUS/CPS/DROPDOWN/TOTAL`,
  `STATUS/NOCLIP_ACCURACY/DROPDOWN/PREFIX`.
- Best Run status fields are covered in [best-run-feature.md](best-run-feature.md).

## Evidence — hooks

Status labels are drawn in the level lifecycle, not through a dedicated render hook
in the recovered cluster. The relevant hooks are:

- `PlayLayer::postUpdate` (`0x1801992B0`) accumulates progress/time globals.
- `PlayLayer::updateProgressbar` (`0x1800A83B0`, `0x180165040`) is hooked twice
  and converges on `FUN_1800AB850`.
- `PlayLayer::toggleProgressbar` (`0x180190E50`) converges on the same helper.

## Pattern

Status labels are configuration-driven: positions and units are stored as typed
config values, while the lifecycle hooks only provide timing/progress. This
matches the nxUI model where rows are layout handles and text comes from the
definition manager.

## Open questions

- UNVERIFIED: which functions read each `STATUS/FIELDS/*` key and format the
  label text.
- UNVERIFIED: the complete set of status field keys beyond the observed 24.
- UNVERIFIED: whether status labels are updated every frame or only on
  lifecycle events.
- UNVERIFIED: the exact format-string grammar used by `STATUS/FIELDS/8/FORMAT`.

## Evidence - tag 424 and the attempt label (2026-10-26)

Tag 424 (`0x1A8`) is the third `PlayLayer::resetLevel` hook in the v9.0.0 block. Registration
`0x1800B198F` (`ba a8 01 00 00`), callback `FUN_1801726B0` (`0x1801726B0..0x180172708`, no
post-install work), detour `0x180172720`, installer `0x1800AB850` with
`[rsp+0x20]=0xFFFFFFE0`, `[rsp+0x28]=1`. Registration and detour re-checked 2026-10-26 with
`scripts/tag_callback_map.py` and `scripts/callback_hooks.py`; body listed with capstone over
`_original.dll`.

After the original runs, the detour recomputes an attempt number and rewrites a label:

```
0x18017273D  call 0x1801A4300             ; original resetLevel
0x180172742  mov  esi, 0x168
0x180172747  add  rsi, [rdi+0x38D8]       ; label node inside the object at +0x38D8
0x18017274E  mov  rax, [rdi+0x878]        ; attempt counters
0x180172755  mov  ecx, [rax+0x2F0]
0x18017275B  sub  ecx, [rax+0x2F4]
0x180172761  mov  [rax+0x2F8], ecx        ; cache the difference
0x1801727B5  lea  rax, [0x180575E74]      ; "Attempt {}"
0x1801727E4  call 0x18000AE30             ; format helper
0x1801727FD  call [rax]                   ; label->setString(formatted)
```

So every reset stores `[layer+0x878]+0x2F0 - [layer+0x878]+0x2F4` at `+0x2F8` and prints it as
`"Attempt {}"`.

Candidates are the v9.0.0 row "Show Total Attempts" and, less likely because of the version,
the v8.0.1-alpha.5 row "Attempt Status". The subtraction argues against a stored total being
displayed unchanged, so "total" is not established.

**What the arithmetic says (2026-10-28).** All three dwords live in the same structure
(`[layer+0x878]`), two of them are subtracted and the result is cached in the third. A cache of
a difference is the signature of a *recomputed* count, not of a stored total being printed
unchanged: a "show the stored total" feature would read `+0x2F0` and format it. That is a
positive argument against the v9.0.0 "Show Total Attempts" wording and in favour of a
per-attempt-window counter, i.e. the v8.0.1-alpha.5 "Attempt Status" candidate, but it does
not name the fields.

UNVERIFIED: what the three dwords at `+0x2F0`/`+0x2F4`/`+0x2F8` hold; whether
`[layer+0x38D8]+0x168` is a host label or one the mod created; which row the tag belongs to.
Closing this needs a runtime read of `+0x2F0`/`+0x2F4` across a reset, or a host-side
reference to the same structure (`GeometryDash.exe` is not in this repo).

## Evidence - velocity and death label types (2026-10-28)

`FUN_18024E8D0` (RVA `0x24E8D0`, `.pdata` `0x18024E8D0..0x18024FB1D`) is the status-label
formatter. It dispatches on a packed four-byte label type stored in the label record, and two
of those type strings are exactly the v9.1.0-beta.1 "Velocity & Death options to Status
labels" wording.

| Type | Compare site | What the branch renders |
| --- | --- | --- |
| `jump` | `0x18024EB06 mov ecx, 0x706d756a` | jump count, gated by tag `0x321` |
| `velocity` | `0x18024EBEB movabs rcx, 0x797469636f6c6576` | `({:.2f}, {:.2f})`, or `({:.2f}, {:.2f}), ({:.2f}, {:.2f})` when `[PlayLayer+0x422] == 1` |
| `dead` | `0x18024EC21 cmp dword ptr [rax], 0x64616564` | the string `"Alive"`, or `"Dead"` when `[PlayLayer+0x324A] != 0` |

(All three VAs from a capstone linear scan of `.text` for the immediate/`movabs` operands;
`FUN_18024E8D0` bounds from `scripts/func_bounds.py`.)

**Velocity.** Per player the value is `[player + 0x9F4] * [player + 0x7B8]` (a float speed
times a double factor), unless `[player + 0xB70] == 1`, in which case the already-computed
double at `[player + 0xAF8]` is used. The x/y pair printed alongside comes from
`[player + 0x9A0]`. Player 1 is `[PlayLayer + 0xDA0]` and player 2 is `[PlayLayer + 0xDA8]`;
the two-player branch is selected by `[PlayLayer + 0x422]`, the same byte the `position` type
uses.

**Death.** The type string is `dead`, the branch prints one of two literals and reads nothing
else, so the option is a label *type* (the row's "Death" option), not a toggle with its own
tag: `[PlayLayer + 0x324A]` is the death flag the vanilla player already maintains.

**Conclusion.** Both halves of the v9.1.0-beta.1 row are accounted for by label types in
`FUN_18024E8D0`, with field-level evidence and no settings key of their own. The row is a
UI/formatting addition, so it has no tag to bind.

**Method note - immediate scans collide with OpenGL constants.** A scan for the immediate
`0x303` returns six sites; five are `glBlendFunc(0x302, 0x303)` / `setBlendFunc({0x302,
0x303})` pairs (`0x1801729CF`, `0x180172A21`, `0x18019AE90`, `0x1801DD9C2`, `0x1801E2762`), and
only `0x18024F2AC` passes it to `FUN_18002DE50`, the Tag-lookup accessor (FNV-1a-64 into the
map at `registry + 0xB0/0xC0/0xD8`, called by `getConfigVar`). The discriminator must be the
call shape, not the immediate alone.

**Open questions.**

- UNVERIFIED: the settings row that writes tag `0x303` (the decimal-place gate on the generic
  percentage branch at `0x18024F2AC`), and whether it is the same key as the `0x1AD`/`0x1AE`
  pair used by the progress bar.
- UNVERIFIED: the unit of the velocity factor at `[player + 0x7B8]`, and whether `0x9F4` is a
  speed multiplier or a step length; naming either needs a GD image.
