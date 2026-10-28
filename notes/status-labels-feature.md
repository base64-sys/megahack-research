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

## Evidence - the label-type table and the v7.0 Level Status family (2026-10-28)

`FUN_1800AF1C0` (RVA `0xAF1C0`, `.pdata` `0x1800AF1C0..0x1800AF7C6`) builds the status-label
type table: an array of 19 `{type string, id_a, id_b}` records passed to
`FUN_1800B54D0(&DAT_1808014D8, 0x13, ...)`. `FUN_18024E8D0` then dispatches on the same type
strings, so the two artefacts agree on the vocabulary by construction.

| Type | Definition ids (`FUN_1800AF1C0`) | Formatter compare | What the branch renders |
| --- | --- | --- | --- |
| `message` | `0x2D9`, `0x2DA` | `0x18024EDB2 mov ecx, 0x7373656d` | `FUN_18002D710(registry, out, 0x2DC)` - the custom message text |
| `testmode` | `0x2DF`, `0x2E0` | `0x18024EB9F movabs rcx, 0x65646f6d74736574` | reads `[PlayLayer+0x3230]`, prints the literal `Testmode` |
| `cheats` | `0x2E5`, `0x2E6` | not read this pass | `.` or empty, gated by tag `0x2E8` and `safe_mode::hasCheatedInAttempt()` |
| `fps` | `0x2EC`, `0x2ED` | `0x18024E949 xor ecx, 0x7066` | frame counter, suffix gated by tag `0x2EF` |
| `cps` | `0x2F5`, `0x2F6` | `0x18024E962 xor ecx, 0x7063` | click rate; suffixes gated by `0x2F9`/`0x2FA` |
| `best_run` | `0x300`, `0x301` | `0x18024EBB2 movabs rcx, 0x6e75725f74736562` | not read this pass |
| `noclip_acc` | `0x309`, `0x30A` | not read this pass | not read this pass |
| `noclip_health` | `0x310`, `0x311` | not read this pass | not read this pass |
| `attempts` | `0x317`, `0x318` | `0x18024EBC5 movabs rcx, 0x7374706d65747461` | tag `0x1A8` picks `[PlayLayer+0x3084]` or `[[+0x878]+0x2F0] - [+0x2F4]` cached at `+0x2F8`; tag `0x31A` adds the `"Attempt {}"` prefix |
| `jumps` | `0x31E`, `0x31F` | `0x18024EB06 mov ecx, 0x706d756a` | `[PlayLayer+0x3970]`; tag `0x321` selects `"{} Jumps"` over `"{}"` |
| `percent` | `0x324`, `0x325` | `0x18024EDC9 mov ecx, 0x63726570` | `FUN_1801A4CB0(PlayLayer)` clamped to `[0,100]`; tag `0x1AD` plus decimals `0x1AE` produce `"{{:.{}f}}%"` |
| `level_time` | `0x329`, `0x32A` | not read this pass | not read this pass |
| `session` | `0x32E`, `0x32F` | `0x18024EDE0 mov ecx, 0x73736573` | `FUN_180194240(&x)`, then `x/10000/1000.0` into the time formatter `FUN_18027E990` |
| `clock` | `0x334`, `0x335` | `0x18024EB1C mov ecx, 0x636f6c63` | `_Xtime_get_ticks()` into `FUN_18027E700`, suffix gated by tag `0x337` |
| `frame` | `0x33B`, `0x33C` | `0x18024EB32 mov ecx, 0x6d617266` | `FUN_180205E80(FUN_1801299D0())`; tag `0x33E` selects `"Frame {}"` |
| `position` | `0x341`, `0x342` | `0x18024EBD8 movabs rcx, 0x6e6f697469736f70` | per-player x/y pair (see the velocity section for the two-player byte) |
| `velocity` | `0x346`, `0x347` | `0x18024EBEB movabs rcx, 0x797469636f6c6576` | see the section above |
| `dead` | `0x34B`, `0x34C` | `0x18024EC21 cmp dword ptr [rax], 0x64616564` | `Alive` / `Dead` |
| `replay_state` | `0x350`, `0x351` | `0x18024EC99 movabs rcx, 0x735f79616c706572` | replay playback state |

**Conclusion - the six v7.0 rows.** Six of the v7.0 `Level Status ...` changelog rows are label
types rather than toggles, and each has a definition record and a formatter branch:
"Level Status Attempts" is `attempts`, "Level Status Clock" is `clock`, "Level Status Custom
Message" is `message`, "Level Status Jumps" is `jumps`, "Level Status Percentage" is
`percent`, and "Level Status Session Time" is `session`. The names match the type strings
one-for-one, and no other row in the universe uses those words for a different mechanism.

**Open questions.**

- UNVERIFIED: what the two ids in each definition record mean. For `jumps` they are
  `0x31E`/`0x31F` while the formatter's unit toggle is `0x321`, and for `message` they are
  `0x2D9`/`0x2DA` while the text key is `0x2DC`; the offsets suggest a row-id and an option-id
  pair, but the writer of the pair was not read.
- UNVERIFIED: "Level Status Only Total Clicks" and `cps`. The `cps` branch reads the same
  counters as `fps` and has two suffix tags, but nothing in the branch distinguishes a
  "total clicks" mode from a rate, so the row stays `cluster`.
- UNVERIFIED: `testmode`, `best_run`, `noclip_acc`, `noclip_health`, `level_time`,
  `position` and `replay_state` were not read this pass; they are listed for the type
  vocabulary only.
