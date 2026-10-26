# Show Trajectory feature cluster

- **Question** — which code and settings implement Mega Hack's Show Trajectory?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

| Setting key | Observed value | Role |
| --- | --- | --- |
| `LEVEL/SHOW_TRAJECTORY/DROPDOWN/ITERATIONS` | 130 | Number of trajectory iterations |
| `LEVEL/SHOW_TRAJECTORY/DROPDOWN/THICKNESS` | 0.4 | Trajectory line thickness |

Changelog features include:

- "Added Show Trajectory iterations/thickness options"
- "Added CBS Tick Highlighting in Show Hitbox Trail"

## Evidence — implementation

- `ShowTrajectory` is a class with vtables at `0x180181C80`
  (`FUN_180181C80`) and `0x180182810` (`FUN_180182810`).
- `FUN_180182390` (`0x180182390`) compares against
  `s___AVShowTrajectory___1807f7f10`, so it identifies the class by RTTI.
- The constructor at `0x180181C80` allocates the object and installs vtables.

## Open questions

- UNVERIFIED: which settings tags correspond to `ITERATIONS` and `THICKNESS`.
- UNVERIFIED: which render hook draws the trajectory and whether it reuses
  `CCDrawNode` or a custom primitive.
- ~~UNVERIFIED: whether "CBS Tick Highlighting" is a separate setting or a mode~~ —
  measured 2026-10-26: it is a setting read by the half-tick trajectory builder, but the
  tag-to-name binding is still open. See the lead below.
  of Show Trajectory.
- UNVERIFIED: whether trajectory rendering is gated by Show Hitboxes.

## Lead — the half-tick trajectory builder and tag 361 (2026-10-26)

This answers part of the open question above: the sub-tick code exists and is measurable, but
which of the two remaining trail settings tag 361 carries is **not** settled, so the changelog
row stays `cluster`.

Artifact: `absolllute.hackmega_original.dll` (SHA-256
`1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`). Method: capstone over the
file-backed range plus a `.text` scan for `mov edx,0x169` / `mov r8d,0x169`.

- Tag **361** (`0x169`). Exactly one config read in the whole image: `0x180182B71`
  `mov edx,0x169` immediately before `call 0x18002E4C0` at `0x180182B79`. The other
  `0x169` immediates are arguments to unrelated host calls in the `0x1803C…`-`0x1804…`
  range, not tag reads.
- The containing function starts at `0x180182AA0` and has **no `.pdata` record**. It takes
  `(this, level, xmm2, player_index r9d)`, saves xmm6-xmm13 and the nonvolatiles, and
  `sub rsp,0x168`. It swaps a player object into the layer slot
  (`[rdi + index*8 + 0x140]` saved, then written to `[level + index*8 + 0xDA0]`).
- The tag value is consumed as an **integer count, floored at one**:
  `movsxd rcx,eax; shl rax,0x20; mov [rbp-8],rcx; cmp rcx,1; adc rcx,0`.
- The loop then steps a simulated player `count` times per call, appending 8-byte trajectory
  points through `[r13+8]` (`movlps [rdi],xmm7; add qword [r13+8],8`), calling `0x1802086C0`,
  reading the player position through `[player vtable+0xC8]`, clearing the player's
  `CCArray` at `+0xA38`, and swapping `[player+0x9E4]`/`[player+0x7E4]`. That is a sub-tick
  (half-tick) sampler feeding the hitbox trail.
- Callers of `0x180182AA0`: `0x180181847`, `0x180181861`, `0x1801819BC`, `0x1801819D6` — all
  inside the same trail-update region, so the builder is used only from the trail path.
- The settings tree has exactly three keys in the trail family: `HIGHLIGHT_HALFTICKS` (True),
  `REQUIRE_SHOW_HITBOXES` (False; already bound to tag 227 from the draw gate) and
  `UNCAPPED_EDITOR_LENGTH` (True). Tag 361 is one of the two that remain.

Why this is a lead and not a promotion: the changelog wording is "CBS Tick Highlighting in
Show Hitbox Trail", which reads as `HIGHLIGHT_HALFTICKS`, but `UNCAPPED_EDITOR_LENGTH` is
equally unbound and the integer-count consumption does not distinguish a boolean from a
length. Binding by name alone would be a guess.

UNVERIFIED: whether tag 361 is `HIGHLIGHT_HALFTICKS` or `UNCAPPED_EDITOR_LENGTH`; which tag
the other key uses; the meaning of the `0x18002E4C0` accessor's return type for this tag
(it is used as an int here, unlike the double reads through `0x180003D80`); and whether the
8-byte point records are positions only or carry a highlight flag.

## Evidence — CBS Tick Highlighting (2026-10-26)

Row `v9.1.0-beta.3 | Added CBS Tick Highlighting in Show Hitbox Trail`. Two runtime tags carry
it and both are read only in the hitbox-trail code: **361** (`0x169`) is the integer sub-tick
count consumed by the sampler, **362** (`0x16A`) is a double converted to float in the
adjacent highlight draw path.

### Tag 361 — sub-tick count

Read at `0x180182B79` inside the half-tick sampler `0x180182AA0` (no `.pdata` record):

```
0x180182B6F  call 0x18008A510          ; settings context
0x180182B74  mov edx, 0x169            ; tag 361
0x180182B79  call 0x18002E4C0          ; integer accessor -> eax
0x180182B7E  movsxd rcx, eax
0x180182B81  shl rax, 0x20
0x180182B85  mov [rbp-8], rcx
0x180182B89  cmp rcx, 1
0x180182B8D  adc rcx, 0                ; floor the count at 1
```

The loop at `0x180182D25` then runs that many iterations (`r14 = 0 .. count-1`), each one
stepping a simulated player (`[player vtable+0xC8]` position read, `[player+0x9E4]`/
`[player+0x7E4]` swapped, `[player+0xA38]` cleared through `CCArray::removeAllObjects`),
calling `0x1802086C0` and appending an 8-byte point record with `movlps [rdi],xmm7;
add qword [r13+8],8`. That is a sub-frame trajectory sampler: it emits the intermediate
positions the trail draws between two game ticks, which is what "CBS tick highlighting"
describes.

The count is an integer, so it is not `HIGHLIGHT_HALFTICKS` (a boolean in the settings tree)
— which key feeds 361 is therefore still open, and the row is promoted on the mechanism, not
on a key binding.

### Tag 362 — highlight colour

Read at `0x180182595` inside `0x180182550` (pushes rsi/rdi/rbx, `sub rsp,0xB0`, /GS cookie
from `[rip+0x674AC0]`):

```
0x18018258E  call 0x18008A510          ; settings context
0x180182593  mov edx, 0x16A            ; tag 362
0x180182595  call 0x180003D80          ; double accessor
0x18018259A  xorps xmm6, xmm6
0x18018259D  cvtsd2ss xmm6, xmm0       ; double -> float
0x1801825A1  mov rcx, rsi
0x1801825A4  call 0x1805330B0          ; colour / vertex helper
0x1801825A9  lea rbx, [rdi+0x160]
```

The function then copies `[rdi+0x178]`, `[rdi+0x188]`, `[rdi+0x198]`, `[rdi+0x1A8]` into a
colour/vertex structure alongside flags at `[rdi+0x1B8]`/`[rdi+0x1BC]`, so the converted
double is consumed as a colour or alpha component of the highlight. `0x180182550` is next
to the sampler (`0x180182AA0`), which is why the two tags are read as one feature; the
proximity is inference, not a call edge.

### Tag 226 — MAX_DRAW (corroborating the family)

`0x180180487` reads tag 226 (`0xE2`) through the same integer accessor, then
`rdx = [0x18080C7F8]` (live point count), `sub rdx, rax`, `jbe skip`, and trims through the
ring at `0x18080C7D0`/`0x18080C7F8`. The only integer in the
`LEVEL/SHOW_HITBOXES_TRAIL` family is `MAX_DRAW = 1024`, so tag 226 = MAX_DRAW is a strong
inference (recorded as such, not as a proven key binding). It matters here because it shows
the trail code's three numeric reads (226 max draw, 228 opacity, 361 sub-ticks) are each a
distinct trail setting rather than aliases.

UNVERIFIED: which JSON keys feed tags 361 and 362 (the family's three booleans —
`HIGHLIGHT_HALFTICKS`, `REQUIRE_SHOW_HITBOXES` (bound to 227) and `UNCAPPED_EDITOR_LENGTH` —
do not include a numeric or colour key, and the two remaining booleans are not located in
the trail code either); that `0x180202BD0`'s third argument is vertex alpha rather than
width; the meaning of `[rdi+0x160..0x1BC]`; and whether the highlight colour is 362 or a
constant with 362 acting as opacity.
