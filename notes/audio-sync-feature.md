# Audio sync / deafen feature cluster

- **Question** — which settings and code implement Mega Hack's Auto Music Sync and
  Auto Deafen features?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — settings keys

From [settings-contents.md](settings-contents.md):

Auto Music Sync:

- `LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/NO_LEGACY_SYNC` (bool, True)
- `LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/THRESHOLD` (int, 40)

Auto Deafen:

- `LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_IN_PRACTICE` (bool, True)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_WITH_STARTPOS` (bool, True)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/UNDEAFEN_ON_PAUSE` (bool, True)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_AT` (decimal, 35.0)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/UNDEAFEN_AT` (decimal, 100.0)
- `LEVEL/AUTO_DEAFEN/DROPDOWN/SHORTCUT` (bytes, [45])

## Evidence — UI registration

- `FUN_180062940` (`0x180062940`) registers:
  - `HACK_AUTO_MUSIC_SYNC`
  - `HACK_AUTO_DEAFEN`
  - `SHORTCUT_AUTO_DEAFEN`
  - `SPINNER_AUTO_MUSIC_SYNC`

## Changelog features

- "Fixed Auto Music Sync not accounting for Music Offset"
- "Added Auto-Update" is unrelated but appears in the same changelog cluster.

## Open questions

- UNVERIFIED: which functions read each Auto Music Sync / Auto Deafen tag.
- UNVERIFIED: how the threshold and deafen percentages are applied to GD's
  audio engine.
- UNVERIFIED: whether Auto Deafen uses a hook on `FMODAudioEngine::update` or
  only settings-driven behaviour.
- UNVERIFIED: the meaning of `SHORTCUT_AUTO_DEAFEN`'s byte value `[45]`.

## Evidence — Corrective Music Sync (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18019C420` (`0x18019C420`) registers three things:

1. `FUN_18019C6D0` as a change listener for tag 0x1F7 (503).
2. `FUN_18019C7E0` as a hook on `FMOD::ChannelControl::setPitch`.
3. `FUN_18019C800` as a hook on `FMODAudioEngine::setMusicTimeMS` (RVA 0x5C1C0).

`FUN_18019C6D0` (`0x18019C6D0`) is the corrective scaler: when `*param_2 == 1`
and `isHackEnabled(tag 0x1F6 = 502)` is true, calls `FUN_18019C400()` (returns
`DAT_1807F9420`, the corrective pitch) and scales all FMOD channels' pitch by
`corrective / previous`. Stores the new value in `DAT_1807F9428`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1F6 | 502 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Enable corrective pitch scaling |
| 0x1F7 | 503 | listener-registered (tag 0x1F7) | (not in static list) | Trigger for corrective recalculation |
| — | — | global `DAT_1807F9420` | — | Corrective pitch value |

UNVERIFIED: which JSON keys bind to tags 0x1F6 and 0x1F7; the semantics of
`FUN_18019C7E0` and `FUN_18019C800`; the exact corrective pitch algorithm.

## Evidence — Pitch Shifter (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_18018A740` (`0x18018A740`) registers `FUN_18018A800` as a change
listener for tag 0x1CC (460). When tag 460 changes:

`FUN_18018A800` checks `isHackEnabled(HACK_PITCH_SHIFTER = 459, 0x1CB)`. If
enabled, calls `FUN_18020DA00(value)` which applies the pitch to audio channels
0–4 via `FUN_18020DAD0`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1CB | 459 | `isHackEnabled` (Map B) | `HACK_PITCH_SHIFTER` | Master toggle |
| 0x1CC | 460 | (listener-registered, value is double) | (not in static list — runtime-assigned) | Pitch value (applied to FMOD channels 0–4)

UNVERIFIED: which JSON key binds to tag 0x1CC; the exact pitch range; whether
the listener registration uses `hackpro::Config`'s change-listener mechanism.

## Evidence — Auto Music Sync (2026-10-26)

This answers the first open question above for Auto Music Sync: the tags are read by one
`PlayLayer::postUpdate` hook and one worker, both read in full here. Auto Deafen is **not**
covered by this section.

| Field | Value |
| --- | --- |
| Tag | 326 (`0x146`, `HACK_AUTO_MUSIC_SYNC`), static label store `0x180062AFF` |
| Registration | `0x1800B1EB6` (`notes/hook-registrations.md`) |
| Callback | `0x1801751E0` (order `0xFFFFFFE0`) |
| Target | `PlayLayer::postUpdate`, RVA `0x3B4BA0` |
| Detour | `0x180175250`, body ends `ret` at `0x1801752FC` (0xAC bytes) |
| Installer | `0x1800AB9F0` |

Callback shape: enable byte `[rdx]` -> `0x180108960` context -> `cmp bl,1` -> module base
`0x18010A250` -> resolver `0x18010A370(edx=0x3B4BA0)` -> `[rsp+0x20]=0xFFFFFFE0`,
`[rsp+0x28]=1`, `r8`=detour, `r9`=`0x18054552B` "PlayLayer::postUpdate" ->
`0x1800AB9F0`. The disable branch tail-jumps `0x180108A80` with the detour.

### Detour `0x180175250`: six gates, then the worker

```
0x180175250  push rsi; sub rsp,0x60; /GS cookie 0x1807F7040
0x180175267  call 0x1801A37C0            ; cached original postUpdate (cache 0x18080FC98)
0x18017526C  call 0x18015A2F0            ; level-state check
0x180175271  test al,al; jne 0x1801752E3 ; true -> skip
0x180175275  cmp byte [rsi+0x309E],0; jne 0x1801752E3   ; platformer -> skip
0x18017527E  call 0x180129AB0            ; second state check
0x180175286  test al,al; jne 0x1801752E3 ; true -> skip
0x18017528A  lea rcx,[rsp+0x48]; r8d=8; call 0x18015AFD0 ; dictionary group 8
0x18017529D  cmp qword [rsp+0x50],0; jne 0x1801752E3    ; non-empty -> skip
0x1801752A5  ... group 7 ... [rsp+0x40]
0x1801752C0  ... group 6 ... [rsp+0x30]
0x1801752DB  mov rcx,rsi; call 0x180175310                ; sync worker
```

`0x15AFD0` is the shared `absolllute.megahack` user-dictionary reader already documented for
Smart StartPos and the StartPos Switcher (it returns `{first, count}`). So sync is suppressed
while dictionary groups 6, 7 or 8 hold anything — what those groups represent is not
established (UNVERIFIED).

### Worker `0x180175310`

1. `0x180128CF0` returns the audio engine (cached singleton: base from `0x18010DDC0`, slot
   `base+0x6C2E88`). `rcx = [engine+0x228]`, then
   `FMOD::ChannelGroup::getChannel(group, 0, &channel)` (`0x180533200`) and
   `FMOD::Channel::getPosition(&pos_ms, 1)` — i.e. the music channel's current position in
   milliseconds. Both results are error-checked; a non-OK result returns.
2. Reads boolean tag **328** (`0x148`, `0x18002DE50`). When true:
   `ebx = [[layer+0xDB0]+0x14C]` (speed index), `0x18015AFD0(layer,&pair,3)`,
   `[layer+0xDA0]` (player 1) vtable `+0xD8`, then `0x1802046D0(...)`; the returned float is
   the target position. Otherwise: player-1 vtable `+0xC8` -> `[rax]` double, followed by
   `[layer vtable+0x4B8](layer, &double, 0, 0, 0, 0)`.
3. Adds the per-speed offset `[[layer+0xDB0]+0x158]`, adds the engine's `float
   [engine+0x264]` (**music offset**, milliseconds), multiplies by `1000.0`
   (`0x180572AD0`), truncates to int and adds `0x28` (40 ms).
4. Reads double tag **327** (`0x147`, `0x18002E4C0`). Drift is `pos_ms - target_ms`; if
   `|drift| >= tag327` it calls `FMOD::Channel::setPosition(target_ms, 1)` to re-seat the
   music channel. Otherwise it leaves the channel alone.

Tag map for this feature:

| Tag (hex) | Tag (dec) | Accessor | Binding |
| --- | --- | --- | --- |
| 0x146 | 326 | `isHackEnabled` (registration) | master toggle `HACK_AUTO_MUSIC_SYNC` |
| 0x147 | 327 | `getConfigVar<double>` `0x18002E4C0` | `LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/THRESHOLD` (default 40) |
| 0x148 | 328 | `getConfigVar<bool>` `0x18002DE50` | `LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/NO_LEGACY_SYNC` (default True) |

The two key bindings are inferred from count and type: the settings file has exactly two
Auto Music Sync sub-keys, one int threshold and one bool, and the worker reads exactly one
double (compared against a millisecond drift) and one bool (selecting between two position
sources). `NO_LEGACY_SYNC` is true by default and tag 328 selects the branch that uses the
speed index and the player-1 `+0xD8` virtual, but which branch the changelog calls "legacy"
is not proven.

The hardcoded `+ 0x28` also matches the threshold's default of 40, so it may be a leftover
constant rather than a lead-in term — UNVERIFIED.

UNVERIFIED: what `0x18015A2F0` and `0x180129AB0` return; what dictionary groups 3, 6, 7 and 8
hold; the identity of the player-1 vtable slots `+0xC8` and `+0xD8` and of the layer slot
`+0x4B8`; what `[engine+0x264]` is called in the host (read here as the music offset because
the changelog fix "not accounting for Music Offset" matches it); and whether `[engine+0x228]`
is the music channel group rather than another group (FMOD API usage and the name string
`getChannel@ChannelGroup` are the evidence).

## Evidence - No Fade Out Audio and Load Audio to Memory (2026-10-26)

Two v9.0.0 audio rows resolve to one bare-`ret` suppression and one shared condition bundle.
Neither row has a static `HACK_*` label: both toggles are read through the tag registry with
runtime-assigned numbers, which is why the label join never proposed them.

### No Fade Out Audio = tag 477

| Field | Value |
| --- | --- |
| Tag | 477 (`0x1DD`), no static label |
| Registration | `0x1800B3104`, bytes `ba dd 01 00 00` |
| Callback | `FUN_180189890`, body `0x180189890..0x1801898E8` (`ret` at `0x1801898E8`); disable path `0x1801898E9` calls `0x180108A80` |
| Target | `FMODAudioEngine::fadeOutMusic`, RVA `0x5C530` (resolved through `0x18010A250` then `0x18010A370`) |
| Name string | `0x1805798F4` "FMODAudioEngine::fadeOutMusic" |
| Detour | `0x180189900`, one byte: `c3` (bare `ret`) |
| Installer | `0x180189910` (the generic name/symbol installer, `0x180108E70`), `[rsp+0x20]=0xFFFFF428`, `[rsp+0x28]=1` |

The detour never enters the original, so every `fadeOutMusic` call becomes a no-op: no fade
out is started, and whatever music is playing keeps its volume. That is exactly the row's
wording, and the mechanism is unique in the image - a whole-`.text` scan finds `mov
edx,0x5C530` exactly once (`0x1801898AB`) and exactly one reference to the name string
(`0x1801898CF`), so no second fade-suppression path exists.

### Load Audio to Memory = tag 476, inside the {326, 476} condition

| Field | Value |
| --- | --- |
| Tag | 476 (`0x1DC`), no static label; exactly one binding site in the image |
| Condition leaves | `mov edx,0x146` at `0x18018850E` (tag 326, `HACK_AUTO_MUSIC_SYNC`), `mov edx,0x1DC` at `0x180188526` |
| Composite | `0x180107CE0` at `0x18018854F`, operator `r8d=1`; wrappers `0x1801084E0` / `0x180108610` |
| Registration | `0x1800915B0` at `0x18018858A` |
| Action body | `0x180188790` (the `{326,476}` action installs all three hooks below) |

The action installs:

| # | Host routine | RVA / resolution | Detour | Installer | Name string |
| --- | --- | --- | --- | --- | --- |
| 1 | `PlayLayer::resumeAndRestart` | `0x3BA3C0` | `0x180188A40` | `0x180174EC0` (`[rsp+0x20]=0xBD8`) | `0x1805797F3` |
| 2 | `FMOD::System::createStream` | mangled symbol, resolved by `0x18010A5A0` | `0x180188B10` | `0x180188CD0` | `0x18057980F` |
| 3 | `FMODAudioEngine::preloadMusic` | `0x5C7C0` | `0x180188B40` | `0x180188FF0` | `0x18057988D` |

`createStream` is one of only two targets in the image resolved by mangled symbol (the other
is `CCMotionStreak::stopStroke`): the action builds the string
`?createStream@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z`
in a 0x70-byte heap buffer (`0x1803A50B4`, immediates `0x6C6C642E646F6D66` = "fmod.dll" plus
four 16-byte chunks copied to `+0x20`, `+0x30`, `+0x40`, `+0x50`) and passes it to the
resolver at `0x1801888A4`.

### The memory-loading mechanism

`0x180188B10` (the `createStream` detour):

```
0x180188b10  sub  rsp, 0x28
0x180188b14  cmp  byte ptr [rip + 0x683ddd], 0     ; byte [0x18080C8F8]
0x180188b1b  je   0x180188b32                      ; flag clear -> original stream path
0x180188b1d  mov  rax, [rsp + 0x50]                ; 5th argument (out sound)
0x180188b22  mov  [rsp + 0x20], rax
0x180188b27  call 0x180533270                      ; ?createSound@System@FMOD@@...
0x180188b2c  add  rsp, 0x28
0x180188b31  ret
```

`0x180188B32` is `add rsp,0x28; jmp <trampoline>` for the original `createStream`. So while
the flag is set, a requested *stream* is created with `System::createSound` instead - the
whole file is decoded into memory rather than streamed from disk.

`0x180188B40` (the `preloadMusic` detour) is what sets the flag: it writes
`byte [0x18080C8F8]=1` at `0x180188B5B`, makes an SSO-aware `std::string` copy of the path,
calls `0x1801132D0(engine, &path, r8d, r9d)`, and clears the flag at `0x180188C20` before
returning. Closure proof: the flag has exactly three reference instructions in `.text` - the
read at `0x180188B14`, the write of 1 at `0x180188B5B` and the write of 0 at `0x180188C20`.
Only this detour can enable the substitution, so ordinary streaming outside a preload is
untouched.

Binding angles: "Load Audio to Memory" is the only row about audio loading, its key
`UNIVERSAL/HACK_LOAD_AUDIO_TO_MEMORY` is a plain boolean toggle, tag 476 sits in the v9.0.0
block beside 477, and the row's wording describes exactly this substitution. The pairing with
tag 326 is consistent: auto music sync repositions the music channel, which needs the audio
decoded in memory, so the bundle is installed when either toggle is on.

### UNVERIFIED

- The operator semantics of the `{326, 476}` composite (value 1). "Either toggle installs the
  bundle" is an inference from the two inputs being independent features; whether the
  evaluator treats 1 as AND or OR is not established, so whether enabling auto music sync
  alone installs the memory-loading hooks is unproven.
- What `0x1801132D0` is (read here as a replacement preload that routes through the swapped
  `createStream`, not observed), what host setting "0128" means in the `resumeAndRestart`
  detour, and what `[0x180200050 + 0xD9]` tracks.
- Which half of the bundle the `resumeAndRestart` detour serves; it touches the player, layer
  node `[+0x3540]` and two engine calls, none of them audio loading.
- The JSON paths that write tags 476 and 477 (settings keys live in the profile, not in the
  image), and no runtime observation of either suppression.
