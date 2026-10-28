# Auto Deafen feature

- **Question** — which code implements the "Auto Deafen" audio toggle?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence

`HACK_AUTO_DEAFEN` (tag 279, `0x117`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b1dc8`:

```
0x1800b1dc8  mov  edx, 0x117          ; tag = HACK_AUTO_DEAFEN
0x1800b1dcd  mov  rcx, rax            ; resolver
0x1800b1dd0  mov  r8, rsi             ; context structure
0x1800b1dd3  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180174a80` (`0x180174a80`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayLayer::resetLevel`
  (RVA `0x3B8EB0`) via `FUN_1800AB850`, using the string
  `"PlayLayer::resetLevel"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x117 | 279 | (registered via FUN_18008f910) | `HACK_AUTO_DEAFEN` | Master toggle (Auto Deafen) |

## Method

1. Searched for byte pattern `ba 17 01 00 00` (mov edx, 0x117) in `_original.dll`
   using a Python byte scan. Found 1 occurrence in the Mega Hack code range
   (`0x1800b1dc8`).
2. Disassembled `FUN_1800afae0` around `0x1800b1dc8` with `pe_disasm.py` to confirm
   the tag value and the call to `FUN_18008f910`.
3. Analyzed `FUN_180174a80` (the callback) with REA/Ghidra 12.1.2 — hooks
   `PlayLayer::resetLevel` (RVA `0x3B8EB0`).

## Open questions

- UNVERIFIED: which JSON key binds to tag 0x117.
- UNVERIFIED: the exact behavior of the hook (whether it vetoes or observes level resets).
- UNVERIFIED: how PlayLayer::resetLevel relates to audio deafening.


Correction (2026-10-26): a leaked tool-call transcript tail had been appended to this
file and was not research content. It has been removed; nothing above it was changed.

## Evidence - the Auto Deafen decision and its four flags (2026-10-28)

The row was already mapped to tag 279; this pass names the settings rows that sit beside it
and reads the decision itself. Method: the label-map record layout recorded in
[hook-registrations.md](hook-registrations.md#how-the-static-label-map-stores-a-tag-2026-10-28)
gives the `CHECKBOX_*` names, a whole-`.text` capstone scan for the tag immediates gives the
read sites, and both detours are disassembled from the image.

| Tag | Label-map record | Read site | Role |
| --- | --- | --- | --- |
| `0x117` (279) | `HACK_AUTO_DEAFEN`, tag store `0x180062AB1` | registration `0x1800B1DC8` | master toggle (already documented) |
| `0x119` (281) | `CHECKBOX_DEAFEN_IN_PRACTICE`: name bounded by len store `0x1800649BF` (`0x1B`), chunks `0x1800649CF`/`0x1800649DA`, NUL `0x1800649E4`; tag `0x1800649E8` | `0x180174D0E` `mov edx,0x119` + `FUN_18002DE50` | allow deafening while the practice byte is set |
| `0x11A` (282) | `CHECKBOX_DEAFEN_WITH_STARTPOS`: len `0x180064A11` (`0x1D`), NUL `0x180064A36`, tag `0x180064A3A` | `0x180174D29` | allow deafening when a startpos is active |
| `0x11B` (283) | no label (decimal) | `0x180174D45` `mov edx,0x11B` + `FUN_180003D80` (double) | lower bound of the deafen window |
| `0x11C` (284) | no label (decimal) | `0x180174D59` + `FUN_180003D80` | upper bound of the deafen window |
| `0x11D` (285) | `SHORTCUT_AUTO_DEAFEN`, tag store `0x180065471` | `0x180174E27` `r8d=0x11D` + `FUN_18002EE40` | key vector the change is delivered through |
| `0x118` (280) | no label | `0x180174DBE` in `0x180174DB0`, result `cl=0` | reachable only as a forced-undeafen path; UNVERIFIED which |

The nested names are readable directly from the record: `CHECKBOX_DEAFEN_IN_PRACTICE` is 27
bytes, so the record stores length `0x1B` at `0x1800649BF`, copies 16 bytes at offset `0x0B`
and 16 bytes at offset `0x0`, writes the NUL at `+0x1B` (`0x1800649E4`) and only then stores
the tag `0x119` at `0x1800649E8`. Same shape for `CHECKBOX_DEAFEN_WITH_STARTPOS` (29 bytes,
`0x1D`). The two `CHECKBOX_*` names match the `LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_IN_PRACTICE`
and `DEAFEN_WITH_STARTPOS` keys in the settings tree, so the label -> key join needs no
guessing.

### The decision: `0x180174CD0` (PlayLayer::postUpdate detour)

Registrar `0x180174A80` installs two detours, `PlayLayer::resetLevel` (RVA `0x3B8EB0`,
name string `0x180545515`) and `PlayLayer::postUpdate` (RVA `0x3B4BA0`), both order
`-32`/`0xFFFFFFE0`. The `postUpdate` body is `0x180174CD0` (`0x180174CD0..0x180174DA5`):

```
0x180174CE3  call cached original postUpdate
0x180174CE8  registry = FUN_18008A510()
0x180174CF0  rax = [layer+0xDA0];  test rax,rax; je 0x180174D05
0x180174CFC  cmp byte [rax+0x9C0],0; jne 0x180174D79      ; player gone -> leave audio alone
0x180174D05  cmp byte [layer+0x31F0],1; jne 0x180174D1F  ; practice byte
0x180174D0E  if (!getConfigVar<bool>(0x119)) goto skip   ; DEAFEN_IN_PRACTICE
0x180174D1F  cmp qword [layer+0x3188],0; je 0x180174D3A  ; startpos active?
0x180174D29  if (!getConfigVar<bool>(0x11A)) goto skip   ; DEAFEN_WITH_STARTPOS
0x180174D3A  percent = getCurrentPercent(layer)
0x180174D45  deafenAt   = getConfigVar<double>(0x11B)
0x180174D59  undeafenAt = getConfigVar<double>(0x11C)
0x180174D6A  if (deafenAt >= undeafenAt) goto skip
0x180174D8A  cl = (percent < undeafenAt)                  ; setb at 0x180174D8D
0x180174DA0  jmp 0x180174E00
```

So the predicate is `deafenAt <= percent < undeafenAt`, evaluated once per `postUpdate`, and
`[PlayLayer+0x31F0]` is the practice-mode byte the gate reads - the same byte the Utilities
action `0x35B` flips ([ui-addons.md](ui-addons.md)).

### The shared tail: how the mute actually happens

Both detours tail-jump to `0x180174E00`, which latches the state byte
`[0x18080C581]` (`0x180174E14 cmp byte [rip+0x697766],cl`): unchanged -> return, changed ->
store `cl`, read tag `0x11D` with `FUN_18002EE40`, and call `FUN_18019EF30`, which is a
`keybd_event` press/release loop over the `DWORD` vector (`FUN_18019EF30`: `keybd_event(*k,0,0,0)`
for each, then `keybd_event(*k,0,2,0)` for each). The `resetLevel` detour `0x180174C80`
passes `cl=0` through the same tail, so a level reset clears the latch and releases the
shortcut.

**Conclusion.** Auto Deafen does not touch FMOD directly: it computes a boolean window test in
a `PlayLayer::postUpdate` detour and, when the boolean flips, synthesises the user's
`SHORTCUT_AUTO_DEAFEN` keys through `keybd_event`, so the actual mute is whatever the game's
keybinding does. The four sub-rows are tags 281-284 plus the shortcut key 285, and the
practice indicator they depend on is `PlayLayer+0x31F0`.

**Open questions.**

- UNVERIFIED: what tag `0x118` gates (`0x180174DB0` forces `cl=0` after reading it); the
  name `UNDEAFEN_ON_PAUSE` in the settings tree makes a pause hook the likely caller, but no
  pause hook is attached to this tail.
- UNVERIFIED: which settings rows write 283/284 - they have no static label, and only the
  read sites above exist, so the join rests on the double type plus the 35.0/100.0 defaults.
- UNVERIFIED: whether `[layer+0x3188]` is the startpos object pointer rather than a flag; the
  test is `!= 0` on a qword.
## The v6.3.0 wording is the same tag (2026-10-28)

The changelog names Auto Deafen twice: v6.3.0 (Full) "Added Auto-Deafen" and
v8.0.0-alpha9 "Added Auto Deafen". Tag 279 (`0x117`, `HACK_AUTO_DEAFEN`) has exactly
one `mov edx,0x117` site in `.text` - the registration at `0x1800B1DC8` inside
`FUN_1800AFAE0` (capstone scan of the whole section, `0x1000`-`0x533FE6`) - so a second
Auto Deafen mechanism does not exist and the hyphen is a respelling, not a new feature.
