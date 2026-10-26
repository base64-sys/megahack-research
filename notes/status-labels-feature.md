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

UNVERIFIED: what the three dwords at `+0x2F0`/`+0x2F4`/`+0x2F8` hold; whether
`[layer+0x38D8]+0x168` is a host label or one the mod created; which row the tag belongs to.
