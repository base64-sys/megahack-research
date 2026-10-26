# Gameplay hooks cluster

- **Question** — which gameplay paths does Mega Hack hook, and what shared patterns do
  they use?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Filter `recovered/functions/_hook_sites.tsv` for GD methods in `PlayerObject`,
   `GJBaseGameLayer`, and the collision / button / update families.
2. Read the shared helpers named by those rows.
3. Cross-check against `notes/hook-dispatch.md` and `notes/host-methods.md`.

## Evidence — core gameplay hook sites

| Registrar VA | GD method | Target | Callback VA | Callback kind | Helper |
| --- | --- | --- | --- | --- | --- |
| `0x180172160` | `PlayerObject::update` | RVA `0x388D80` | `0x1801721D0` | label | `0x1800AAC20` |
| `0x180173920` | `PlayerObject::update` | RVA `0x388D80` | `0x180173990` | function | `0x1800AAC20` |
| `0x1801990B0` | `PlayerObject::collidedWithObjectInternal` | RVA `0x391A70` | `0x180199440` | function | `0x180199730` |
| `0x1801990B0` | `PlayerObject::collidedWithSlopeInternal` | RVA `0x38F810` | `0x1801994B0` | function | `0x180199A50` |
| `0x180165890` | `PlayerObject::ringJump` | RVA `0x398C00` | `0x180165B60` | function | `0x180165DC0` |
| `0x180165890` | `PlayerObject::hitGround` | RVA `0x39BF30` | `0x180165BC0` | function | `0x180166010` |
| `0x180173D00` | `PlayerObject::playerDestroyed` | RVA `0x397D40` | `0x180173E30` | function | `0x180167B20` |
| `0x180172DE0` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180172E50` | function | `0x180172EC0` |
| `0x180177B00` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180177D90` | function | `0x180172EC0` |
| `0x180197020` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180197270` | function | `0x180172EC0` |
| `0x180198BF0` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x180198CD0` | function | `0x180172EC0` |
| `0x18019CA80` | `GJBaseGameLayer::update` | RVA `0x237850` | `0x18019E560` | label | `0x180172EC0` |
| `0x180167210` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180167280` | function | `0x180167330` |
| `0x180174590` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180174600` | function | `0x180167330` |
| `0x180176EA0` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180176F10` | function | `0x180167330` |
| `0x18017A710` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x18017A780` | function | `0x180167330` |
| `0x180181540` | `GJBaseGameLayer::collisionCheckObjects` | RVA `0x214960` | `0x180181AA0` | function | `0x180167330` |
| `0x1801746F0` | `GJBaseGameLayer::processCommands` | RVA `0x239C60` | `0x180174760` | function | `0x180174920` |
| `0x1801801D0` | `GJBaseGameLayer::processCommands` | RVA `0x239C60` | `0x180180240` | function | `0x180174920` |
| `0x18019CA80` | `GJBaseGameLayer::processCommands` | RVA `0x239C60` | `0x18019E490` | function | `0x180174920` |
| `0x180197020` | `GJBaseGameLayer::updateCamera` | RVA `0x23BCF0` | `0x180197440` | label | `0x180172EC0` |
| `0x18015C3F0` | `GJBaseGameLayer::shouldExitHackedLevel` | RVA `0x20B050` | `0x18015C460` | label | `0x18015B120` |
| `0x180198BF0` | `GJBaseGameLayer::handleButton` | RVA `0x2338E0` | `0x180198D00` | function | `0x180198EF0` |
| `0x18019CA80` | `GJBaseGameLayer::handleButton` | RVA `0x2338E0` | `0x18019E4C0` | function | `0x180198EF0` |

## Pattern

- `GJBaseGameLayer::update` is the central gameplay choke point: it is registered
  five times (`0x180172DE0`, `0x180177B00`, `0x180178C60`, `0x180197020`,
  `0x180198BF0`, `0x18019CA80`) and all converge on helper `FUN_180172EC0`.
- `GJBaseGameLayer::collisionCheckObjects` is registered five times
  (`0x180167210`, `0x180174590`, `0x180176EA0`, `0x18017A710`,
  `0x180181540`) and all converge on helper `FUN_180167330`.
- `GJBaseGameLayer::processCommands` is registered three times
  (`0x1801746F0`, `0x1801801D0`, `0x18019CA80`) and all converge on helper
  `FUN_180174920`.
- `PlayerObject::update` is registered twice (`0x180172160`, `0x180173920`)
  and both converge on helper `FUN_1800AAC20`.
- `GJBaseGameLayer::handleButton` is registered twice (`0x180198BF0`,
  `0x18019CA80`) and both converge on helper `FUN_180198EF0`.

The repeated registration of the same method from different registrars is the
same pattern seen in the input cluster: Mega Hack observes the same gameplay event
from several ownership or timing contexts, then funnels each family through one
shared helper.

## Evidence — No Wave Trail Behind (recovered 2026-10-24 via REA/Ghidra 12.1.2 + pe_disasm)

`HACK_NO_WAVE_TRAIL_BEHIND` (tag 414, `0x19E`) is registered as a hook in the large
initializer `FUN_1800afae0` (`0x1800afae0`) at `0x1800b182a`:

```
0x1800b182a  mov  edx, 0x19E          ; tag = HACK_NO_WAVE_TRAIL_BEHIND
0x1800b182f  mov  rcx, rax            ; resolver
0x1800b1832  mov  r8, rsi             ; context structure
0x1800b1835  call 0x18008f910         ; register hook with tag
```

The callback is `FUN_180172280` (`0x180172280`), which takes `(param_1, param_2)`:
- If `*param_2 == 1`: registers a hook on `PlayerObject::activateStreak`
  (RVA `0x3A0090`) via `FUN_1800AB230`, using the string
  `"PlayerObject::activateStreak"`.
- If `*param_2 != 1`: unregisters the hook via `FUN_180108a80()`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x19E | 414 | (registered via FUN_18008f910) | `HACK_NO_WAVE_TRAIL_BEHIND` | Master toggle (No Wave Trail Behind) |

UNVERIFIED: which JSON key binds to tag 0x19E; the exact behavior of the hook
(veto vs observe).

## Open questions

- UNVERIFIED: which settings tags gate each gameplay hook. The recovered callbacks
  shown here do not expose their tags.
- UNVERIFIED: the behaviour of shared helpers `FUN_180172EC0`,
  `FUN_180167330`, `FUN_180174920`, `FUN_1800AAC20` and `FUN_180198EF0`;
  only their call shape and convergence are recorded here.
- UNVERIFIED: whether `shouldExitHackedLevel` is a veto hook or an observer.
- UNVERIFIED: whether the five `collisionCheckObjects` registrations differ by
  object class, layer, or timing.

## Evidence — Frame Stepper (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180173FD0` (`0x180173FD0`) is a hook registrar that conditionally installs
two UILayer hooks based on two Map B tags:

- **Tag 357 (0x165)** — runtime-assigned Map B tag (not in the 85-entry static
  list). If set, the registrar takes the "enabled" path.
- **Tag 302 (0x12E)** — `HACK_FRAME_STEPPER` (static map entry #21). If set
  (and tag 357 is not), the registrar also takes the "enabled" path.

Enabled path (either tag set): registers `LAB_1801740C0` on
`UILayer::onPause` (RVA `0x4CDF50`) and `LAB_1801740E0` on
`UILayer::handleKeypress` (RVA `0x4CD830`), both via `FUN_180174100`/`FUN_1801742E0`
with enum arg `0xFFFFF428` (-3096, Geode convention).

Disabled path (neither tag): unregisters both hooks via `FUN_180108A80`.

UNVERIFIED: the exact behaviour of `LAB_1801740C0` and `LAB_1801740E0` (they are
code labels inside `FUN_180173FD0`, not separate functions); what tag 357
represents (candidate: a "Frame Stepper enabled in pause" sub-option); whether
the pause hook vetoes or observes.

## Evidence — 0% Practice Complete (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`HACK_0_PRACTICE_COMPLETE` (tag 296, 0x128) is entry #0 in the 85-entry static
label map (store VA `0x1800629C3`). It is a Map B hack-flag tag.

The only code reference is `FUN_1800917B0` (`0x1800917B0`), which is a virtual
method of `hackpro::Config` (vtable at 0x180544FF0, secondary vtable). It calls
`FUN_18008F420(registry + 0xD8 - 0x128, *param_2)` — i.e., it reads the Map B
flag for tag 296 through the config interface.

No gameplay code reads tag 296 directly through the known accessors. The feature
is likely a config value read by GD or by other code through the exported API
(`isHackEnabled` at `0x180072FF0`).

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x128 | 296 | `isHackEnabled` (Map B) via config vtable | `HACK_0_PRACTICE_COMPLETE` | Master toggle

UNVERIFIED: which gameplay code reads this tag and what it does; the JSON key
that binds to it; whether this is "0% Practice Complete" or "0% Practice Mode
Completion".

## Evidence — TPS Bypass (recovered 2026-10-23 via REA/Ghidra 12.1.2)

`FUN_180196C30` (`0x180196C30`) registers/unregisters `FUN_180196CA0`
(`0x180196CA0`) as a hook on `GJBaseGameLayer::getModifiedDelta`
(RVA `0x2377B0`), depending on a boolean parameter.

`FUN_180196CA0` computes a frame time delta:

- If `isHackEnabled(tag 0x1EA = 490)` is true (master toggle):
  - If `PlayLayer+0x32A4 < 1` (no pending frames): returns `1.0 / customTPS`
    where customTPS = tag 0x1E6 (486, double accessor, default 240.0) if
    `isHackEnabled(tag 0x1E7 = 487)` is true and the value is valid, else 240.0.
  - If `+0x32A4 >= 1`: decrements the counter, returns 0.0 (frame skip).
- If tag 0x1EA is false but tag 0x1E7 is true: computes a stepped frame time
  using `PlayLayer+0x3250` (accumulated remainder) and `PlayLayer+0x330` (scale
  factor), with `round()` quantization.
- If neither: calls `FUN_180115B70` (GD's default `getModifiedDelta`).

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1EA | 490 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Master toggle (TPS Bypass) |
| 0x1E7 | 487 | `isHackEnabled` (Map B) | (not in static list — runtime-assigned) | Custom TPS sub-toggle |
| 0x1E6 | 486 | `FUN_180003D80` (double accessor) | (not in static list) | Custom TPS value (default 240.0)

UNVERIFIED: which JSON keys bind to tags 0x1E6, 0x1E7, 0x1EA; the semantics of
`PlayLayer+0x32A4` (frame counter) and `+0x3250`/`+0x330`; whether this is
"TPS Bypass" or "Legacy Mode to Speedhack".

## Evidence — four labelled tags found by the full-section scan (2026-10-25)

The registration scan was originally bounded to `0x1800AFAE0`–`0x1800B2000` and missed 46
registrations. Four of the recovered tags have a static label that matches a changelog row
verbatim, so the tag↔feature binding needs no inference:

| Tag | Static label | Registration | Callback | Hooks |
| ---: | --- | --- | --- | --- |
| `0x59` | `HACK_INSTANT_COMPLETE` | `0x1800B25B9` | `0x1801789E0` | `PlayLayer::postUpdate` 0x3B4BA0, detour `0x180178B10`; `PlayLayer::levelComplete` 0x3A7A80, detour `0x180178BC0`; `PlayLayer::updateTimeWarp` 0x3AB9A0, detour `0x180178BE0` |
| `0x12A` | `HACK_CHECKPOINT_LIMIT_BYPASS` | `0x1800B2092` | `0x180176250` | `PlayLayer::storeCheckpoint` 0x3B74A0, detour `0x1801762C0` |
| `0x1C3` | `HACK_ALLOW_LOW_VOLUME` | `0x1800B2EA7` | `0x180186100` | `PauseLayer::musicSliderChanged` 0x37E130 and `sfxSliderChanged` 0x372A90, `OptionsLayer::musicSliderChanged` 0x372990 / `sfxSliderChanged` 0x372A90, detours `0x180186290`/`0x1801862C0`/`0x1801862F0`/`0x180186370` |
| `0x1CD` | `HACK_FAST_CHESTS` | `0x1800B300C` | `0x180187820` | `RewardUnlockLayer::init` 0x3CDDF0, detour `0x180187CE0`; `playRewardEffect` 0x3CEDE0, detour `0x180187D00`; `showCloseButton` 0x3D0480, detour `0x180187D20`; two symbol-resolved hooks |

All four callbacks follow the usual shape: `movzx`/`cmp` on `*param_2`, the `== 1` path
installs with the detours above, and the other path unregisters through `FUN_180108A80`.

### The "scoped flag" detour idiom

Instant Complete and Fast Chests do not suppress their target. Their detours set a global
byte, call the original work, then clear the byte:

```
; Fast Chests, RewardUnlockLayer::init detour 0x180187CE0
0x180187CE4  mov  byte [rip+0x684BF5], 1     ; -> 0x18080C8E0
0x180187CEB  call 0x1801AAAE0                ; original work
0x180187CF0  mov  byte [rip+0x684BE9], 0     ; -> 0x18080C8E0
0x180187CFB  ret
```

| Feature | Flag global | Writers | Reader |
| --- | --- | --- | --- |
| Instant Complete | `0x18080C6C8` | `0x180178BC4` (set) / `0x180178BD0` (clear) in the `levelComplete` detour | `0x180178BED` (`cmp byte [flag],1`, same detour) |
| Fast Chests | `0x18080C8E0` | `0x180187CE4` / `0x180187CF0` (`init`) and `0x180187D04` / `0x180187D10` (`playRewardEffect`) | `0x180187D40` / `0x180187D60` (`cmp byte [flag],0`, same callback) |

**Correction (2026-10-25):** the Instant Complete flag was first recorded as
`0x18080C8C8`, a transposition of `0x18080C6C8`. A full `.text` scan for rip-relative
references to the wrong address found only unrelated instructions (`lea r8`/`lea rdx`/
`mov word [..],0x3333`), which is what exposed it; the right address has exactly the
expected set / clear / compare triple. The two flags are therefore **not** adjacent.

Both readers are inside the detour that writes the flag, so each flag is scoped to its own
wrapper rather than read by game code. **UNVERIFIED:** what the wrapper changes while the
flag is set, since the reader's branch was not followed this pass.

### Checkpoint Limit Bypass rewrites an argument

`PlayLayer::storeCheckpoint` (0x3B74A0) is tail-replaced by `0x1801762C0`:

```
0x1801762CC  mov  rcx, [rcx+0x3888]     ; PlayLayer -> sub-object
0x1801762D3  call 0x180532E10           ; UNVERIFIED callee
0x1801762D8  mov  rdx, [rsi+0xE58]      ; checkpoint object -> field
0x1801762DF  mov  rax, [rdi]            ; PlayLayer vtable
0x1801762E2  mov  rax, [rax+0x478]      ; original storeCheckpoint
0x1801762E9  mov  rcx, rdi
0x1801762F2  jmp  rax
```

The second argument is replaced with the value at `[checkpoint+0xE58]` before the original
method is entered, and `[this+0x3888]` is touched first. That is consistent with replacing
the checkpoint's stored limit/time with a value that bypasses the cap, but **UNVERIFIED:**
the identity of `0x180532E10`, the type of `[checkpoint+0xE58]`, and whether the cap is
enforced inside `storeCheckpoint` or downstream.

### Allow Low Volume recomputes the slider value

Each of the four slider detours recomputes the float argument and tail-jumps into the
volume path. The music variant:

```
0x180186299  call 0x180128CF0          ; singleton/context
0x1801862A4  call 0x1801FF9C0          ; slider -> float (rcx = param_2)
0x1801862AC  movaps xmm1, xmm0
0x1801862B5  jmp 0x180129980           ; volume setter
```

The detour passes a different float to the setter than the original would have; the exact
remapping (and whether it is a clamp, a scale, or a flooring of the low end) is
**UNVERIFIED** — the two callees were not identified.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x059 | 89 | `isHackEnabled` (Map B) | `HACK_INSTANT_COMPLETE` | Instant Complete master toggle |
| 0x12A | 298 | `isHackEnabled` (Map B) | `HACK_CHECKPOINT_LIMIT_BYPASS` | Checkpoint limit bypass |
| 0x1C3 | 451 | `isHackEnabled` (Map B) | `HACK_ALLOW_LOW_VOLUME` | Allow volume below the vanilla floor |
| 0x1CD | 461 | `isHackEnabled` (Map B) | `HACK_FAST_CHESTS` | Fast chest rewards |

UNVERIFIED: which JSON keys bind to these four tags; the semantics of the two flag globals;
what `[checkpoint+0xE58]` holds; and the volume remapping.

## Evidence — Main Menu Gameplay (2026-10-25)

`HACK_MAIN_MENU_PLAY` (tag 457, `0x1C9`; entry #31 in the static label map, store VA
`0x180063529`) is registered in `FUN_1800AFAE0` at `0x1800B308D`; the callback is
`FUN_180189260`. It hooks three methods of the menu player layer, all resolved by name:

| Target | RVA | Detour | Installer |
| --- | ---: | --- | --- |
| `MenuGameLayer::tryJump` | `0x331F70` | `0x180189390` | `0x180189450` |
| `MenuGameLayer::update` | `0x332240` | `0x1801893A0` | `0x180189450` |
| `MenuGameLayer::ccTouchBegan` | `0x333600` | `0x180189400` | `0x180189630` |

The three detours implement one small state machine around a single flag byte at
**`0x18080C920`** (three rip-relative references, all inside these detours):

```
0x180189400  sub  rsp, 0x28
0x180189404  add  rcx, -0x140              ; ccTouchBegan's node -> the menu layer
0x18018940D  mov  rcx, [rcx+0x1B0]         ; the menu icon object
0x180189419  cmp  byte [rcx+0x9BE], 0      ; icon state A
0x180189422  cmp  byte [rcx+0x9BB], 0      ; icon state B
0x180189432  call 0x1801A85A0              ; start controlling the icon
0x18018943C  mov  byte [0x18080C920], 1    ; gameplay active
0x180189443  mov  al, 1                    ; consume the touch
```

If either icon-state byte is non-zero the detour returns `0` without starting anything, so
the touch falls through to the menu. `update` (detour `0x1801893A0`) checks the same flag
first, calls `0x18019EF10`, and while the flag is set advances the icon through
`0x1801A8920([layer+0x1B0], 1)` before clearing the flag at `0x1801893E7`.
`tryJump`'s detour is a bare `ret` (`0x180189390`), which removes the menu icon's automatic
jump while the player is driving it.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x1C9 | 457 | `isHackEnabled` (Map B) | `HACK_MAIN_MENU_PLAY` | Master toggle (Main Menu Gameplay) |

UNVERIFIED: what `0x18019EF10` and `0x1801A85A0`/`0x1801A8920` are (names not recovered);
what the icon-state bytes `+0x9BB`/`+0x9BE` mean; why `tryJump` must be suppressed rather
than left to run; which JSON key binds to tag 0x1C9.

## Evidence — Show Layout (2026-10-25)

`HACK_SHOW_LAYOUT` (tag 260, `0x104`, static-map entry #63 store VA `0x180064095`) is
registered at `0x1800B2B5C` (`ba 04 01 00 00`); the callback is `FUN_18017F0F0`, which
installs six same-image hooks. Every friendly-name string was read from the callback's
`lea r9` operand, so the table is not relying on a single memory of the names:

| Target | RVA | Detour | Installer | Name string |
| --- | ---: | --- | --- | --- |
| `PlayLayer::resetLevel` | `0x3B8EB0` | `0x18017F330` | `0x1800AB850` | `0x180545515` |
| `PlayLayer::applyEnterEffect` | `0x3B1660` | `0x18017F750` | `0x18017FAA0` | `0x180577FB0` |
| `PlayLayer::applyCustomEnterEffect` | `0x3B0970` | `0x18017F7C0` | `0x18017FD40` | `0x180577FCC` |
| `EffectGameObject::triggerObject` | `0x4A5F30` | `0x18017F830` | `0x180168350` | `0x1805758A1` |
| `GameObject::addGlow` | `0x18DEE0` | `0x18017F8E0` | `0x18017FF90` | `0x180577FEE` |
| `GameObject::saveActiveColors` | `0x1A08D0` | `0x18017FA60` | `0x18017EB00` | `0x180578002` |

Literal observations from the detour bytes:

- **`resetLevel`** calls the original (`0x1801A4300`), reads config tag `0x106` (262) through
  `0x18002DE50`, and on a true result reads tag `0x107` (263) as an int via `0x18002E4C0`
  and passes `&local` with it to `0x1801F15E0`. So the toggle has two Map A sub-keys, not one.
- **`applyEnterEffect` / `applyCustomEnterEffect`** both require `[object+0x3A0] == 7` and
  then skip the effect for object IDs `0x26`, `0x2C`, `0x2ED` and `0x64F`, falling through to
  a virtual call for everything else.
- **`triggerObject`** is a jump-table dispatch: it accepts IDs `> 0x630` with
  `id - 0xB57 <= 0x80` and jumps through a 4-byte table at `0x180575020`, otherwise returns
  without calling anything.
- **`addGlow`** takes a `std::string` by reference: the detour reads length at `[arg+0x10]`,
  capacity at `[arg+0x18]`, buffer at `[arg]`, builds a copy (SSO when length `<= 0xF`), then
  clears `[this+0x516]`, calls `0x18014A5A0(this, &copy)`, restores the byte and frees the
  copy. The earlier suspicion that this target was mis-assigned is **withdrawn**: the name
  string, the callback's `lea r8` operand and the body all agree that the hooked function
  takes a string.
- **`saveActiveColors`** does the same scoping around a different callee: test
  `[this+0x516]`, clear it, call `0x18014A4D0`, restore it to 1.

Both `addGlow` and `saveActiveColors` therefore bracket one call with the same byte at
`+0x516`, which is why they are two hooks rather than one: the toggle suppresses the
colour/glow bookkeeping while still letting the underlying call run.

UNVERIFIED: which of the six hooks produces the visible layout; the meaning of tags 262/263
and their defaults (now measured as one of four Show Layout colour pairs, see the
colour-options section below); the jump-table destinations; and what `+0x516` actually controls.
Nothing was inferred about the visual result from the feature's name.

## Evidence — Show Layout colour options (2026-10-26)

Extends the Show Layout section above. The `resetLevel` detour (`0x18017F330`, installed by
`FUN_18017F0F0` with installer `0x1800AB850`, name string `0x180545515`, hook RVA `0x3B8EB0`)
does not stop at tags 262/263: it carries four independent `(bool, colour)` config pairs and
writes one colour per pair into objects fetched from the host through `0x180144ED0`. That is
what the v9.0.8 row "Added Show Layout colour options" is.

Body extent `0x18017F330..0x18017F739` (`ret` at `0x18017F739`, 0x40A bytes, no `.pdata`),
`/GS` cookie `0x1807F7040`, failure funclet at `0x18017F73A`.

| Bool tag | Colour tag | Ids passed to `0x180144ED0` | Fallback bytes | Fallback colour |
| --- | --- | --- | --- | --- |
| 262 (`0x106`) | 263 (`0x107`) | `0x3E8` (1000) | `0x18080C7B8..BA` | `0x287DFF` |
| 265 (`0x109`) | 266 (`0x10A`) | `0x3E9` (1001), `0x3F1` (1009) | `0x18080C7BB..BD` | `0x0066FF` |
| 271 (`0x10F`) | 272 (`0x110`) | `0x3EA`, `0x3EB`, `0x3EC` (1002-1004) | `0x18080C7C1..C3` | `0x287DFF` |
| 268 (`0x10C`) | 269 (`0x10D`) | `0x3F5`, `0x3F6` (1005, 1006) | `0x18080C7BE..C0` | `0xFFFFFF` |

Per pair the detour calls the original `resetLevel` first (`call 0x1801A4300`), reloads the
settings context (`0x18008A510`), reads the bool with `0x18002DE50`, and on a true result reads
the colour with `0x18002E4C0` and decomposes it through `0x1801F15E0`. That helper is six
instructions: `out[0] = v >> 16`, `out[1] = (v >> 8) & 0xFF`, `out[2] = v & 0xFF`. On a false
result the same three bytes are copied from a static fallback triple. Then
`0x180144ED0([layer+0x950], id)` returns an object and the detour stores
`[obj+0x3A] = out[0]`, `[obj+0x3B] = out[1]`, `[obj+0x3C] = out[2]`.

`0x180144ED0` is another delayed-bind thunk, not a mod function: `recovered/functions/_wrappers.tsv`
lists thunk `0x180144ED0`, resolver `0x180144FA0`, target RVA **`0x25BC00`**, contained in
`FUN_18025B08D+0xB73` (the initialiser at `0x180144F3A..0x180144F4B` does
`lea rcx,[rax+0x25BC00]` and stores the result at `0x180807270`).

The four fallback triples are initialised by one static function at `0x180180170`
(`sub rsp,0x28`; `lea rcx,[0x18080C7A8]`; `call 0x180106DC0`; then the eight immediate stores
`0x180180180..0x1801801B9`). Measured values: `0x287DFF`, `0x0066FF`, `0x287DFF`, `0xFFFFFF`.

Binding to the settings keys: `notes/settings-contents.md` lists exactly four Show Layout
colour pairs, `LEVEL/SHOW_LAYOUT/DROPDOWN/{BG,GROUND,MG,OBJ}/ENABLED` (lines 68-71) and
`.../COL` (lines 186-189), with `COL` defaults BG `2653695` (`0x287EBF`), GROUND `26367`
(`0x0066FF`), MG `2653695`, OBJ `16777215` (`0xFFFFFF`). Two fallback colours match the JSON
defaults exactly, which binds **265/266 = GROUND** and **268/269 = OBJ**. BG and MG share one
default (`0x287EBF`), while the two remaining fallbacks are both `0x287DFF`, so which of
262/263 and 271/272 is BG is **not** proven; the only hint is that pair 262/263 writes id 1000
and 1000 is the background channel in GD's colour numbering.

After the eight writes the detour walks the vector `[layer+0x950]+0x2E0..+0x2E8` (8-byte
stride, count capped at `0x3E7`) and resets every non-null element to white:
`[+0x3A] = 0xFFFF`, `[+0x3C] = 0xFF`, `[+0x40] = 1.0f`, `[+0x54] = 0`, `[+0x78] = 0`,
`[+0x58] = 0`.

UNVERIFIED: what host RVA `0x25BC00` is and what the ids `0x3E8..0x3F6` name (they behave like
colour-channel or colour-trigger ids, but no name string in the image says so); the BG/MG order;
why 272 covers three ids and 266 covers two; what `[+0x40]`, `[+0x54]`, `[+0x78]`, `[+0x58]`
are on the returned object; and whether `[layer+0x950]` is the layer's colour-trigger container.

## Evidence — Respawn Time modifier (2026-10-25)

`HACK_RESPAWN_TIME` (tag 245, `0xF5`; static-map entry #57, store VA `0x180063E55`) is
registered at `0x1800B2AE5` in `FUN_1800AFAE0`. Callback `FUN_18017EF50` installs one hook:

| Target | RVA | Detour | Installer |
| --- | ---: | --- | --- |
| `PlayLayer::destroyPlayer` | `0x3B39D0` | `0x18017EFC0` | `0x180169480` |

The detour supplies the delay instead of calling the original `destroyPlayer`:

```
0x18017EFC8  call 0x1801A3D20              ; the destruction work
0x18017EFCD  call 0x18008A510              ; registry/context
0x18017EFD2  mov  edx, 0xF6                ; tag 246 = the configured time
0x18017EFDA  call 0x180003D80              ; double accessor
0x18017EFDF  divsd xmm0, [0x180549250]     ; 1000.0 (bytes 00 00 00 00 00 40 8f 40)
0x18017EFEA  movapd xmm1, xmm0             ; seconds
0x18017EFF3  jmp  0x18017F000
```

The helper `0x18017F000` turns those seconds into a scheduled action:

```
0x18017F010  movapd xmm6, xmm1             ; seconds
0x18017F01F  mov  edx, 0x1F6   ; tag 502
0x18017F027  call 0x18008F420              ; gate
0x18017F038  mov  edx, 0xF7   ; tag 247
0x18017F040  call 0x18002DE50              ; bool accessor
0x18017F049  mov  edx, 0x1F5  ; tag 501
0x18017F051  call 0x180003D80              ; double accessor -> xmm7
0x18017F067  movsd xmm0, [0x180577F98]     ; 2^-23 (1.1920928955078125e-07)
0x18017F06F  maxsd xmm0, xmm6
0x18017F073  mulsd xmm0, xmm7              ; default multiplier 1.0 (0x180534360)
```

Measured facts: the configured value is tag **246**, divided by **1000.0**, clamped to a
minimum of **2^-23**, multiplied by a factor that is **1.0** unless tags **502** and **247**
are both enabled, in which case the factor is the double at tag **501**. The result is
converted to `float` and scheduled as a cocos2d action.

The three "action constructors" this section first recorded as unnamed are resolvable through
their `FF 25` thunks (`scripts/import_names.py`, method in
[host-abi.md](host-abi.md#resolving-the-ff-25-import-thunks-2026-10-25)):

```
0x18017F062  call 0x180532CA0          ; CCNode::stopActionByTag(0x10)
0x18017F088  call 0x180532390          ; CCCallFunc::create(this, 0x1801A4BE0)
0x18017F093  call 0x1805323A0          ; CCDelayTime::create(seconds)
0x18017F0A1  call 0x1800BADB0          ; varargs shim over CCSequence::createWithVariableList
0x18017F0B4  call [rax+0x40]           ; setTag(0x10)
0x18017F0CD  jmp  0x180532CE0          ; CCNode::runAction
```

So the detour cancels any previous respawn task tagged `0x10`, builds
`CCSequence(CCDelayTime(seconds), CCCallFunc(callback 0x1801A4BE0))`, tags it `0x10` and runs
it. `0x1801A4BE0` is mod code (the selector handed to `CCCallFunc::create`); what it does on
the layer is not read yet.

`FUN_18017EF50` hooks the same RVA (`0x3B39D0`) that No Death Effect also hooks, with the same
`PlayLayer::destroyPlayer` friendly name — two features, one host function, different detours.

*Correction (2026-10-25, later the same day).* The first write-up listed
`0x180532390`/`0x1805323A0`/`0x1800BADB0` and `0x180532CE0` as unidentified constructors. They
are named above; only the selector `0x1801A4BE0` and the tags remain open.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x0F5 | 245 | `isHackEnabled` (Map B) | `HACK_RESPAWN_TIME` | Master toggle |
| 0x0F6 | 246 | `FUN_180003D80` (double) | (runtime-assigned) | Configured respawn time, milliseconds |
| 0x1F5 | 501 | `FUN_180003D80` (double) | (runtime-assigned) | Multiplier used when 502 and 247 are on |
| 0x1F6 | 502 | `FUN_18008F420` gate | (runtime-assigned) | Gate for the multiplier |
| 0x0F7 | 247 | `FUN_18002DE50` (bool) | (runtime-assigned) | Second gate for the multiplier |

UNVERIFIED: the names of tags 246, 247, 501 and 502; whether tags 502/247 are the
"Ignore Speedhack option" named by the changelog; what the `0x1801A4BE0` selector does; and
the JSON keys.

## Evidence — Stop Triggers on Death (2026-10-25)

Tag 415 (`0x19F`, `HACK_STOP_TRIGGERS_ON_DEATH`, static-map entry 71, store VA
`0x1800643C4`) is registered at `0x1800B1AF4`. Callback `FUN_180172DE0` installs one hook:

| Target | RVA | Detour | Installer | Order |
| --- | ---: | --- | --- | --- |
| `GJBaseGameLayer::update` | `0x237850` | `0x180172E50` | `0x180172EC0` | `0xBD8` |

`.pdata` bounds the detour to `0x180172E50..0x180172EBE` (0x6E bytes), and the last
instruction is the five-byte tail jump, so the body below is the whole detour:

```
0x180172E60  call 0x1801ACDB0           ; host 0x17B4A0(), then [result+0x208]
0x180172E65  cmp  rax, rsi              ; is that pointer this layer?
0x180172E68  je   0x180172E7F           ; yes -> check the players
0x180172E6A  mov  rcx, rsi              ; (forward path)
0x180172E6D  movaps xmm1, xmm6          ; dt untouched
0x180172E7A  jmp  0x1801149B0           ; gateway -> GJBaseGameLayer::update
0x180172E7F  mov  rax, [rsi+0xDA0]      ; player slot 1
0x180172E86  test rax, rax
0x180172E89  je   0x180172E94           ; null -> try slot 2
0x180172E8B  cmp  byte [rax+0x9C0], 0
0x180172E92  jne  0x180172EA9           ; set -> skip the update
0x180172E94  mov  rax, [rsi+0xDA8]      ; player slot 2
0x180172E9E  je   0x180172E6A           ; null -> forward
0x180172EA0  cmp  byte [rax+0x9C0], 1
0x180172EA7  jne  0x180172E6A           ; != 1 -> forward
0x180172EA9  mov  rcx, rsi
0x180172EB9  jmp  0x180115B70           ; gateway -> GJBaseGameLayer::getModifiedDelta
```

The two tail targets are the mod's lazy call gateways, and their initialisers name the host
entry point they wrap, which is how the branch is identified rather than assumed:

```
0x180114A20  call 0x18010DDC0           ; cached GetModuleHandleA(NULL)  (see host-abi.md)
0x180114A25  lea  rcx, [rax + 0x237850] ; GJBaseGameLayer::update
0x180114A2C  call 0x180114A80           ; build the callable gateway once
0x180114A31  mov  [0x180805BB0], rax

0x180115BE0  call 0x18010DDC0
0x180115BE5  lea  rcx, [rax + 0x2377B0] ; GJBaseGameLayer::getModifiedDelta
0x180115BEC  call 0x180115C40
0x180115BF1  mov  [0x180806180], rax
```

RVA `0x2377B0` is named `GJBaseGameLayer::getModifiedDelta` in the mod's own hook records
(`mov edx, 0x2377B0` at `0x180196C49` and `0x18019709B`, name string at `0x18057A894`), and
`recovered/functions/_hook_sites.tsv` carries the same pairing from the Ghidra side. Two
independent behaviours confirm the return shape: detour `0x180197250` caches the call's
`xmm0` as a `double` (`movsd [rip+0x675807], xmm0`), and detour `0x180196CA0` returns its own
`xmm0` after consulting tag `0x1EA`.

Measured mechanism: while the current layer is the one the host singleton reports at
`+0x208`, and a player slot's byte at `+0x9C0` passes the test above, `update` is **not
called at all**. The frame instead runs `getModifiedDelta(this, dt)` and returns. The toggle
therefore does not filter triggers inside the update — it substitutes a different host entry
point for the whole level update on those frames. Note the asymmetry in the bytes: slot 1
takes any non-zero value, slot 2 requires exactly `1`.

`FUN_180157180` corroborates that `+0xDA0`/`+0xDA8` are the two player slots: it compares an
incoming object pointer against both and maps it to index 3 or 4 before calling
`FUN_180156400`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x19F | 415 | `isHackEnabled` (Map B) | `HACK_STOP_TRIGGERS_ON_DEATH` | Skip the level update while a player is dead |

UNVERIFIED: the meaning of the byte at `player+0x9C0` (it reads like a dead/destroyed flag —
the mod also writes `1` into it at `0x1801818CA` and copies it at `0x180208E55` — but no host
symbol names it); the identity of host RVA `0x17B4A0`, whose `+0x208` field is compared with
`this`; and whether `getModifiedDelta` has side effects that matter when it stands in for
`update`. Whether the freeze this produces is the whole intended effect, or only the intended
visible part of it, cannot be settled without the host image.

## Evidence — Replay Last Checkpoint (2026-10-25)

Tag 316 (`0x13C`, `HACK_REPLAY_LAST_CHECKPOINT`, static-map entry 56, store VA
`0x180063DF7`) is registered at `0x1800B2A6E`. Callback `FUN_18017ECB0` installs one hook:

| Target | RVA | Detour | Installer | Order |
| --- | ---: | --- | --- | --- |
| `EndLevelLayer::onReplay` | `0x134E80` | `0x18017ED20` | `0x18017ED60` | `0xFFFFFFE0` |

The callback is the usual enable/disable pair — `cmp bl, 1`, resolve `0x134E80` through
`0x18010A370`, `lea r8` = detour, `lea r9` = the `EndLevelLayer::onReplay` name string at
`0x180577F6C`, call the installer; on the other branch it calls `0x180108A80` with the detour
to uninstall. `.pdata` bounds the detour to `0x18017ED20..0x18017ED5E` (0x3E bytes), so this
is the whole body:

```
0x18017ED26  test rdx, rdx              ; the second argument (sender)
0x18017ED29  jne  0x18017ED52           ; non-null -> the original
0x18017ED2B  mov  rsi, rcx
0x18017ED2E  mov  rdi, rdx
0x18017ED31  call 0x1801ACDB0           ; [host 0x17B4A0 + 0x208] = the play layer
0x18017ED3C  cmp  byte [rax+0x31F0], 1  ; level state byte
0x18017ED43  jne  0x18017ED52           ; not set -> the original
0x18017ED45  xor  edx, edx              ; argument = 0
0x18017ED4D  jmp  0x180110DC0           ; gateway -> host RVA 0x1354C0
0x18017ED52  ...
0x18017ED59  jmp  0x180110CF0           ; gateway -> host RVA 0x134E80 (onReplay)
```

Both tail targets are lazy gateways whose initialisers carry the host RVA, and the second one
matches the hooked method exactly, which is what makes the first one readable as *a different
host procedure* rather than a helper:

```
0x180110D61  lea rcx, [rax + 0x134E80]  ; EndLevelLayer::onReplay gateway (0x180110CF0)
0x180110E31  lea rcx, [rax + 0x1354C0]  ; the substitute (0x180110DC0)
```

Mechanism: with the toggle on, a call to `EndLevelLayer::onReplay` whose second argument is
null and whose play layer carries byte `+0x31F0 == 1` is rerouted to host RVA `0x1354C0` with
`edx = 0` instead of the normal replay handler. Everything else reaches the original.

`+0x31F0` is not free-standing: a full-`.text` displacement scan finds exactly ten sites, all
in mod code, and **none of them writes `1`** — the only writes are `0`, and they sit in the
two Practice Complete detours (tag 296, `HACK_0_PRACTICE_COMPLETE`, callback `FUN_18017E0B0`):

| Site | Instruction | Owner |
| --- | --- | --- |
| `0x18017E19E` | `cmp byte [rcx+0x31F0], 1` | Practice Complete, `playEndAnimationToPos` detour |
| `0x18017E1B6` | `mov byte [rsi+0x31F0], 0` | same detour, after vtable `+0x568` |
| `0x18017E22F` | `cmp byte [rcx+0x31F0], 1` | Practice Complete, platformer variant |
| `0x18017E24A` | `mov byte [rsi+0x31F0], 0` | same detour |
| `0x18017ED3C` | `cmp byte [rax+0x31F0], 1` | this feature |
| `0x180155609`, `0x180174D05`, `0x180176F34`, `0x18018579E`, `0x18019024E` | reads only | other detours |

Because the field is read as `1` but never written as `1` in this image, the value has to come
from the host, and the mod clears it after the practice-complete end animation. The name
"Replay Last Checkpoint" therefore rests on the changelog row and on the redirection away from
the start-of-level replay handler; the bytes prove the redirection, not the destination.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x13C | 316 | `isHackEnabled` (Map B) | `HACK_REPLAY_LAST_CHECKPOINT` | Reroute the endscreen replay to a different host procedure |

UNVERIFIED: what host RVA `0x1354C0` does (it is 0x640 bytes after `onReplay` and carries no
name string anywhere in the image, so "replays from the last checkpoint" is the changelog's
word, not the bytes'); what `+0x31F0` means and which host code sets it to 1; and which call
site invokes `onReplay` with a null second argument. One tag and one hook serve all three
changelog wordings (v6.2.0 "mod", v7.0 "to Endscreen", v8.0.0-alpha9), so they are promoted
together rather than counted as three implementations.

## Evidence — the two coin-collection hooks (2026-10-25)

Two changelog rows turned out to be one mechanism entered from two sides: `HACK_AUTO_PICKUP_COINS`
(320, `0x140`) clears the coins a level should have already collected after a reset, and
`HACK_COLLECT_COINS_IN_PRACTICE` (297, `0x129`) collects coin objects during collision checks.
Both end in the **same host routine, RVA `0x2161B0`, called with `(layer, object)`** — the
strongest corroboration either row has, because two unrelated features independently route a
coin-like object into it.

| Feature | Tag | Registration | Callback | Target | Detour | Installer |
| --- | --- | --- | --- | --- | --- | --- |
| Auto Pickup Coins | 320 `0x140` | `0x1800B1F2D` | `FUN_1801754C0` | `PlayLayer::resetLevel` RVA `0x3B8EB0` | `0x180175530` | `0x1800AB850` |
| Collect Coins in Practice | 297 `0x129` | `0x1800B2185` | `FUN_180176EA0` | `GJBaseGameLayer::collisionCheckObjects` RVA `0x214960` | `0x180176F10` | `0x180167330` |

Both detours are **code caves** — `scripts/func_bounds.py` returns no `.pdata` record for
either — so each body below was read with a recursive-descent disassembly that follows both
branches, not with a byte-count guess.

### Auto Pickup Coins (tag 320)

A **pre-hook that suppresses nothing**: it calls the original `resetLevel` first, ignores the
result, and then queues collection work of its own.

```
0x180175530  push rbp/rsi/rdi; sub rsp,0x50; lea rbp,[rsp+0x50]
0x180175544  mov  rsi, rcx                 ; this = PlayLayer
0x180175547  call 0x1801A4300              ; gateway -> host RVA 0x3B8EB0 (original resetLevel)
0x18017554C  call 0x18008A510              ; settings context
0x180175551  lea  rdx,[rbp-0x20] ; mov r8d,0x141 ; mov rcx,rax
0x18017555E  call 0x18002EE40              ; ctx, &int_vector, tag 321 -> flags
0x180175563  lea  rcx,[rbp-0x30] ; mov rdx,rsi ; mov r8d,1
0x180175570  call 0x18015AFD0              ; &ptr_vector, PlayLayer, 1 -> candidates
0x180175575  xor  edi, edi                 ; index = 0
0x180175580  inc  edi                      ; loop back-edge
0x180175582  mov  eax, edi
0x180175588  mov  rcx,[rbp-0x20]           ; flags.begin
0x18017558C  mov  r8,[rbp-0x18] ; sub r8,rcx ; sar r8,2    ; flags length
0x180175597  cmp  rdx,r8 ; cmovb r8,rdx    ; bound = min(rdx,[rbp-0x28])
0x18017559E  cmp  r8,rax ; jbe done
0x1801755A3  cmp  dword [rcx+rax*4],0 ; je next   ; flag 0 -> skip
0x1801755A9  mov  rcx,[rbp-0x30]           ; pointer vector
0x1801755AD  mov  rdx,[rcx+rax*8]          ; candidate object
0x1801755B1  mov  rcx,rsi
0x1801755B4  call 0x180116220              ; gateway -> host RVA 0x2161B0
0x1801755BB  ... vector dealloc ... ; ret
```

Gateway proofs: `0x1801A4364 lea rcx,[rax+0x3B8EB0]` is the only `lea [rax+disp]` occurrence of
that RVA in `.text`, and `0x180116291 lea rcx,[rax+0x2161B0]` caches through
`0x180806310`/`0x180806318` via builder `0x180115190`.

### Collect Coins in Practice (tag 297)

This one does **not** call `collisionCheckObjects` first: it scans the object list, collects
what it keeps, and only then tail-jumps to the original.

```
0x180176F10  push r15/r14/r13/r12/rsi/rdi/rbx; sub rsp,0x20
0x180176F1F  mov esi,r9d ; mov rdi,r8 ; mov rbx,rdx ; mov r14,rcx   ; (this, rdx, r8, count)
0x180176F2B  test r9d,r9d ; jle original
0x180176F34  cmp byte [r14+0x31F0],0 ; je original        ; same byte as Replay Last Checkpoint
0x180176F3E  mov r12d,esi ; xor r13d,r13d ; jmp 0x180176F58
0x180176F58  mov rax,[rdi] ; mov rcx,[rdi+8] ; sub rcx,rax ; sar rcx,3   ; list length
0x180176F66  cmp rcx,r13 ; jbe 0x180176FD1                ; out-of-bounds trap
0x180176F6B  mov r15,[rax+r13*8]                          ; object
0x180176F6F  mov eax,[r15+0x3A0]                          ; type id
0x180176F76  cmp eax,0x1F ; je ok
0x180176F7B  cmp eax,0x16 ; jne next                     ; only 0x1F and 0x16
0x180176F80  cmp byte [r15+0x3D2],0 ; jne next            ; skip if set
0x180176F8A  mov rcx,rbx ; mov rdx,r15 ; call 0x1802045D0 ; predicate(object)
0x180176F95  test al,al ; je next
0x180176F99  mov rcx,r14 ; mov rdx,r15 ; call 0x180116220 ; gateway -> host 0x2161B0
0x180176FA4  mov rcx,r14 ; mov rdx,r15 ; call 0x180115AA0 ; gateway -> host 0x216090
0x180176FB1  original: restore 7 regs; add rsp,0x20; jmp 0x180115840   ; host 0x214960
```

Gateway proofs: `0x1801158C3 lea rcx,[rax+0x214960]` (cached `0x180805F80`/`0x180805F88`) and
`0x180115B11 lea rcx,[rax+0x216090]` (cached `0x180806060`/`0x180806068`).

`0x1802045D0` is an ordinary `.pdata` function that saves and restores bytes at
`[object+0x368]` and `[object+0x2D4]` around a virtual call `[vtable+0x490]`.

### Tag map

| Tag (hex) | Tag (dec) | Accessor | Static-map name | Role |
| --- | --- | --- | --- | --- |
| 0x140 | 320 | `isHackEnabled` (Map B) | `HACK_AUTO_PICKUP_COINS` | Collect already-cleared coins after `resetLevel` |
| 0x129 | 297 | `isHackEnabled` (Map B) | `HACK_COLLECT_COINS_IN_PRACTICE` | Collect coin objects during collision checks in practice mode |

UNVERIFIED: the names of host RVAs `0x2161B0` and `0x216090` (the first is inferred to be the
coin-collection routine from being shared by both features and receiving a coin-like object;
neither appears in `_host_methods.tsv` and no hook-name string refers to them); what object
type ids `0x1F` and `0x16` are; what the bytes at `object+0x3A0`, `object+0x3D2` and
`layer+0x31F0` mean; what the int vector read for tag 321 encodes (the changelog wording
suggests a per-coin option, but the vector's producer was not read); the settings key text
assembled inside `0x18015AFD0` (it builds a 19-character string beginning `absolllu...` at
`0x1805465F4`); and whether `[rbp-0x28]` in the Auto Pickup Coins loop is the pointer-list
length or an independent count.

## Lead — Trigger Value Bypass replaces the popup's value control (2026-10-25)

Not enough to promote the row yet: the hook and its whole body are read, but the site that
lifts the value bound is not located, so this stays a lead and the inventory row stays
`cluster`.

| Field | Value |
| --- | --- |
| Tag | 54 (`0x036`, `HACK_TRIGGER_VALUE_BYPASS`) |
| Registration | `0x1800B067B` (`notes/hook-registrations.md` line 84) |
| Callback | `FUN_180164530` (order `0xFFFFFFE0`) |
| Target | `SetupTriggerPopup::createValueControlAdvanced`, RVA `0x475FD0` |
| Detour | `0x1801645A0`, `.pdata` `0x1801645A0..0x180164799` (0x1F9 bytes) |
| Installer | `0x1801647E0` |

The callback reads the enable flag from `byte [rdx]` and only installs on `1`; the disable
branch is at `0x180164589`. The name string it passes to the installer is `0x18057559D`. No
tag is read inside the detour, so the toggle is install/uninstall, not a per-call check.

Measured body shape:

- the `std::string` argument is copied into a local (`length` at `[r8+0x10]`, SSO flag
  `[r8+0x18] >= 0x10` selects the heap pointer at `[r8]`, `memcpy` through `0x180533B10` of
  `length + 1` bytes);
- the `double` at `[r9]` is copied to `[rbp+8]`;
- every stack argument is reloaded from `[rbp+0xA0]`..`[rbp+0xF8]` (the `float`, `int` and
  `byte` parameters in their original positions) and passed to `0x1801FE660(this, edx,
  &string_copy, &double_copy, <stack args>)`;
- then `cocos2d::CCDictionary::removeAllObjects` is called on `[this+0x328]` and
  `[this+0x330]`;
- the callee's value is returned unchanged (`mov rdi, rax` ... `mov rax, rdi`).

`0x1801FE660` has no `.pdata` record. Disassembling it to its `ret` at `0x1801FE879` finds no
`minsd`, `maxsd`, `minss`, `maxss` or `comisd`, so no clamp lives in the shim either. It
re-copies the same string and double and forwards through a thread-guarded function pointer —
guard `[0x180818B48]`, pointer `[0x180818B40]`, `call rbx` at `0x1801FE806`.

UNVERIFIED: which bound the vanilla `createValueControlAdvanced` enforces and where it is
applied; what the guarded pointer at `[0x180818B40]` resolves to; what the two dictionaries at
`this+0x328`/`+0x330` hold and why emptying them belongs to the bypass. Finding the clamp site
is the acceptance criterion for promoting this row.

## Evidence — Jumpscare (2026-10-25)

One tag, one hook on `PlayLayer::destroyPlayer`, and a self-contained UI effect built from a
PNG and an MP3 inside the mod's own resources. Target, registration, callback and the full
detour body are read, so the row is `mapped`.

| Field | Value |
| --- | --- |
| Tag | 291 (`0x123`, `HACK_JUMPSCARE`), static map entry 28, store `0x180063420` |
| Registration | `0x1800B26A7` |
| Callback | `0x180178D10` (one installer call at `0x180178D5C`) |
| Installer | `0x180169480`, name string `0x180575973` "PlayLayer::destroyPlayer" |
| Target | `PlayLayer::destroyPlayer`, RVA `0x3B39D0` |
| Detour | `0x180178D80`, code cave, body `0x180178D80..0x180178DDC` (`ret`), 0x5D bytes, then `int3` padding |

### Detour body

```
0x180178D80  push rsi; push rdi; sub rsp,0x28
0x180178D86  mov rdi, r8                 ; player argument kept for the compare below
0x180178D89  mov rsi, rcx                ; this = PlayLayer
0x180178D8C  call 0x1801A3D20            ; tail-calls the cached original destroyPlayer
0x180178D91  cmp rdi, [rsi+0x3788]
0x180178D98  je  0x180178DD5             ; no effect for this player
0x180178D9A  mov rcx, rsi
0x180178D9D  call 0x180179AE0            ; gate: settings + random
0x180178DA2  test al, al
0x180178DA4  je  0x180178DD5             ; gate said no
0x180178DA6  call 0x180178DE0            ; build the black overlay + sprite
0x180178DAB  mov rdi, rax
0x180178DAE  mov rcx, [rsi+0x3540]       ; parent node of the play layer
0x180178DB5  mov rax, [rcx]
0x180178DB8  mov rdx, rdi
0x180178DBB  mov r8d, 0x64               ; z-order 100
0x180178DC1  call [rax+0x1B8]            ; addChild(overlay, 100)
0x180178DC7  mov rcx, rdi
0x180178DCA  add rsp,0x28; pop rdi; pop rsi
0x180178DD0  jmp 0x180179260             ; tail-call: play sound and run the animation
0x180178DD5  add rsp,0x28; pop rdi; pop rsi; ret
```

`0x1801A3D20` is the delayed-bind trampoline for the original: it tail-jumps the pointer
cached at `0x18080FD28`, and on first use builds it from `base + 0x3B39D0` (`0x1801A3D99`,
base from `0x18010DDC0`) through `0x1801A3E00` and sets the guard dword `0x18080FD30`. The
same RVA is what the callback handed the installer, so the hook replaces the real
`destroyPlayer` and still runs it.

### Gate — `0x180179AE0(layer)` returns `bool`

Settings context `0x18008A510`, then four tag reads:

| Tag | Accessor | Meaning (inferred from use) |
| --- | --- | --- |
| `0x127` (295) | `0x18002DE50` bool | `CHECKBOX_JUMPSCARE_ONLY_0` / `LEVEL/JUMPSCARE/DROPDOWN/ONLY_FROM_0` |
| `0x126` (294) | `0x180003D80` double | `LEVEL/JUMPSCARE/DROPDOWN/TIME_THRESHOLD` |
| `0x125` (293) | `0x180003D80` double | `LEVEL/JUMPSCARE/DROPDOWN/PERCENTAGE` |
| `0x124` (292) | `0x180003D80` double | `LEVEL/JUMPSCARE/DROPDOWN/PROBABILITY` |

1. **Only from 0** (`0x127` true): returns false unless `[layer+0x3188] == 0` *and*
   `CCArray::count([layer+0x3888]) == 0` (`0x180179B08`..`0x180179B20`), i.e. the run must
   have no checkpoint and nothing collected yet.
2. **Time or percentage threshold**: if byte `[layer+0x309E] != 0`, require
   `tag 0x126 <= [layer+0x39C0]` (double compare at `0x180179B2B`); otherwise call
   `0x1801A4CB0(layer)`, widen its `float` result to `double`, and require
   `tag 0x125 <= that value` (`0x180179B4A`..`0x180179B6A`). Failing either returns false.
3. **Probability**: a call through the IAT slot `0x180644D18` — its import entry at RVA
   `0x64F806` is hint `0x001B` + "rand" — reduced modulo 10000
   (`imul 0x68DB8BAD` / `sar 0x2C` / `imul 0x2710`), compared with `tag 0x124 * 100.0`
   (double `0x180534330` = 100.0): returns `probability*100 > rand()%10000`, so 100.0 means
   always and the unit is percent with two decimals.

### Builder — `0x180178DE0() -> CCLayerColor*`

`operator new(0x228)`, `memset(...,0,0x228)`, constructor `0x1801E7F50`, then the class
pointer at `+0x00` and seven vtable slots at `+0x140`, `+0x148`, `+0x150`, `+0x158`, `+0x160`,
`+0x198`, `+0x1B0` are overwritten with `0x180576C78`, `0x180577098`, `0x1805770F8`,
`0x180577108`, `0x180577128`, `0x180577148`, `0x180577168`, `0x1805771E8`
(`CCLayerColor::initWithColor(0xFF000000)` then runs on that object). On success the sprite
from `0x180178FA0` is stored at `[layer+0x220]`, added with `[vtable+0x1C0]`
(single-argument `addChild`) and autoreleased; on failure `[vtable+8]` is called with `edx=1`
and null is returned.

`0x180178FA0` builds the resource name `"jumpscare.png"` in a `std::string` and passes it with
the module string `"absolllute.megahack"` (`0x1805465F4`) to `0x18010C110`, then
`CCSprite::create(path)` (`0x180531A00`).

### Animation — `0x180179260(layer)`

- `0x1801797D0` lazily loads `"jumpscare.mp3"` (same module string) through `0x180179C30`,
  caching the handle at `0x18080C6F0` behind flag `0x18080C6F8`; the loaded object's `+0x230`
  pointer is then played through `FMOD::System::playSound` (`0x180533250`).
- The layer's size is read through `[vtable+0x130]`, halved componentwise with `(0.5, 0.5)`
  (`0x180549C80`) and passed to the sprite through `[sprite vtable+0xB8]`; the sprite then
  receives the float `0.4` (`0x180572A6C`) through `[vtable+0xA0]`.
- `CCFadeIn(0.02)` (`0x180572A94` = 0.02f) + `CCDelayTime(1.0)` (`0x1805343D0`) +
  `CCRemoveSelf(true)` are combined by `0x1800BADB0` and run on the overlay, so the black
  screen appears, holds for a second and removes itself.
- A second chain, also combined by `0x1800BADB0` and run on the sprite, alternates twelve
  `CCFadeTo(255 / 0)` steps with `CCDelayTime(0.02)`, then runs a `CCScaleTo` sequence with
  the same 0.02 delay and factors from `0x180576C54`..`0x180574BCC`, finishing with
  `CCFadeIn(0.02)`.

UNVERIFIED: the meaning of `[layer+0x3788]` — the compare skips the effect when the dying
player equals it, which reads either as a player-slot filter or as a guard against repeated
deaths; nothing in the image names the field. Also open: whether `+0x309E` is the platformer
flag (the branch switches from percentage to time when it is set) and what percent
`0x1801A4CB0` returns; the names of `PlayLayer+0x3540` (the parent the overlay is added to),
`+0x3188` and `+0x3888` (the only-from-0 test); and what each of the seven overwritten
vtable slots at `+0x140..+0x1B0` overrides.

## Evidence — Practice Bug Fix (2026-10-25)

One tag serves both changelog wordings. This build has a single `PRACTICE_BUG_FIX` label and a
single `LEVEL/HACK_PRACTICE_BUG_FIX` settings key, so the v8.0.0-alpha10b bullet and the
v8.0.1-alpha.5 "improved" bullet are the same toggle at two revisions; both rows are promoted
together, as with Replay Last Checkpoint.

| Field | Value |
| --- | --- |
| Tag | 317 (`0x13D`, `HACK_PRACTICE_BUG_FIX`) |
| Static label | map entry 53, store `0x180063CD1` |
| Settings key | `LEVEL/HACK_PRACTICE_BUG_FIX` (`notes/settings-contents.md`) |
| Registration | `0x1800B288D` |
| Callback | `FUN_18017B620` |
| Installer | `0x1800AB850` (the coin hook uses the same installer on the same target) |
| Target | `PlayLayer::resetLevel`, RVA `0x3B8EB0` |
| Detour | `0x18017B690` (code cave) |
| Name string | `0x180545515` "PlayLayer::resetLevel" |

### Callback

```
0x18017B626  movzx ebx, byte [rdx]        ; enable flag
0x18017B629  call 0x180108960             ; hook context
0x18017B631  cmp bl, 1
0x18017B636  call 0x18010A250             ; module base
0x18017B63B  mov edx, 0x3B8EB0
0x18017B643  call 0x18010A370             ; resolve host RVA -> function
0x18017B650  mov dword [rsp+0x20], 0xBD8  ; installer argument 5
0x18017B648  mov dword [rsp+0x28], 1      ; installer argument 6
0x18017B658  lea r8, [0x18017B690]        ; detour
0x18017B65F  lea r9, [0x180545515]        ; "PlayLayer::resetLevel"
0x18017B66C  call 0x1800AB850
```

The disable branch is at `0x18017B679`. The fourth and fifth stack arguments differ from the
Auto Pickup Coins callback (`FUN_1801754C0`), which installs on the same RVA with
`[rsp+0x20] = 0xFFFFFFE0`; this one passes `0xBD8`. Both pass `1` in the sixth slot. The pair
has to be ordered against each other on one target, so the field is read as an install
order/priority, but the meaning of `0xBD8` itself is UNVERIFIED.

### Detour body — `0x18017B690..0x18017B6C0` (0x31 bytes, `ret` at `0x18017B6C0`)

```
0x18017B690  push rsi; sub rsp,0x20
0x18017B695  mov rsi, rcx                 ; this = PlayLayer
0x18017B698  call 0x1801A4300             ; cached original resetLevel, then:
0x18017B69D  mov rcx, rsi
0x18017B6A0  call 0x18017B6D0             ; newest checkpoint -> CCNode* or null
0x18017B6A5  test rax, rax
0x18017B6A8  je 0x18017B6BA               ; no checkpoint: plain return
0x18017B6AA  mov rcx, rax                 ; checkpoint object
0x18017B6AD  mov rdx, rsi                 ; layer
0x18017B6B5  jmp 0x18017B7A0              ; restore, then return
```

`0x1801A4300` is the standard delayed-bind trampoline for the replaced function: guard
`0x18080FE50`, cached pointer `0x18080FE48`, builder `0x1801A33C0`, and
`lea rcx,[rax+0x3B8EB0]` at `0x1801A4364` (module base from `0x18010DDC0`). The detour is
therefore a post-hook: the vanilla `resetLevel` runs first, then the fix re-applies the
checkpoint state.

### Checkpoint lookup — `0x18017B6D0`

```
0x18017B6E3  mov rcx, [rcx+0x3888]        ; PlayLayer checkpoint CCArray
0x18017B6EA  call CCArray::count
0x18017B6F1  je 0x18017B777               ; empty -> return null
0x18017B6FE  call CCArray::lastObject
0x18017B70B  ... RTTI walk ...
0x18017B734  lea rdi, [0x1806E2710]       ; ".?AVCCNode@cocos2d@@"
0x18017B753  call strcmp
0x18017B76B  add rcx, rsi                 ; adjust to the CCNode subobject
```

The walk is a manual `dynamic_cast<CCNode*>`: it reads the complete-object-locator from the
vtable, walks the class-hierarchy array, `strcmp`s each raw descriptor name against
`.?AVCCNode@cocos2d@@` (string at `0x1806E2710`) and returns the last checkpoint only if the
base is found. Non-checkpoint objects are rejected, so the fix never touches an arbitrary
`CCObject`.

### Restore — `0x18017B7A0`

```
0x18017B7A7  mov rdi, rdx                 ; layer
0x18017B7AA  mov rsi, rcx                 ; checkpoint
0x18017B7AD  lea rdx, [rcx+0x148]         ; player-1 state block
0x18017B7B4  mov rbx, [rdi+0xDA0]         ; player 1
0x18017B7BE  call 0x180207470             ; copy block -> player
0x18017B7C3  call 0x18020AFE0             ; static object at 0x180819680
0x18017B7CB  call 0x18020B880             ; mov eax,[rcx+0xF8]; ret
0x18017B7D0  test eax, eax
0x18017B7D2  je 0x18017B7DF               ; global field 0 -> extra step
0x18017B7D4  cmp byte [rsi+0xC38], 1      ; second player recorded?
0x18017B7DF  mov qword [rbx+0xAF8], 0
0x18017B7ED  call 0x1801A8850             ; cached host RVA 0x39F100, arg = player
0x18017B7FB  add rsi, 0x6C0               ; player-2 state block
0x18017B802  mov rdi, [rdi+0xDA8]         ; player 2
```

- `0x180207470` copies a saved block field by field: `[src+0x50] -> [player+0x560]`,
  `+0x58 -> +0x568`, `+0x80 -> +0x590`, `+0xC0 -> +0x5D0`, `+0x110 -> +0x620`,
  `+0x2C4 -> ...` — the same routine the structure note already lists as a Mega Hack-side
  copy (`notes/gd-structures.md`, `+0x2C4` table).
- When `[0x180819680+0xF8]` is zero the detour additionally clears `[player+0xAF8]` and tail
  calls host RVA `0x39F100` with the player; `scripts`-recorded `_wrappers.tsv` maps
  `0x1801A8850` to that host RVA, so the third angle here is the wrapper table, not a guess.
- Player 2 is restored only when the record's byte `+0xC38` equals `1`. The record itself is
  Mega Hack's: the constructor at `0x18017B850` allocates `0xC40` bytes
  (`mov ecx,0xC40` / `operator new`) and installs vtable `0x180577AC8`; the block at `+0x148`
  and the block at `+0x6C0` are `0x578` bytes apart, and `+0x6C0 + 0x578 = 0xC38`, i.e. the
  byte sits immediately after the second block. Sites `0x18017B8A5` (store 0) and
  `0x18017BBB4` (store 1) are the writers of that flag.

Corroboration from other investigated features: `[layer+0xDA0]`/`[layer+0xDA8]` are the two
player slots and `[layer+0x3888]` is the checkpoint `CCArray` whose count is used as "no
checkpoint yet" in the Jumpscare gate. Both fields are therefore measured twice, from
independent detours.

UNVERIFIED: that the array element is GD's `CheckpointObject` and that this class derives from
`cocos2d::CCNode` (the RTTI check proves the base, not the class); that `+0x148`/`+0x6C0` are
per-player state blocks (inferred from the `0x180207470` copy destinations and the `+0xC38`
sentinel immediately after the second one); what host RVA `0x39F100` does and what
`[0x180819680+0xF8]` means (the object is a zero-fill static, four references in the image, no
name string); and what `player+0xAF8` holds. The changelog says "bug fix" but does not name the
bug; the mechanism above is what the shipped code does, not an identification of RobTop's
original defect.

## Evidence — Smart StartPos (2026-10-25)

One tag, one hook, and four sub-toggles read from the replacement body. Two changelog rows are
promoted on this evidence: v8.0.0-alpha5 "Added Smart StartPos" and v9.0.1 "Added Smart StartPos
toggles (Gamemode, Speed, Size, Mirror)". The v7.1 "Editor Support for Smart StartPos" row stays
`cluster`: the callback installs exactly one hook, on `PlayLayer::resetLevel`, and nothing in
that path touches an editor layer.

| Field | Value |
| --- | --- |
| Tag | 235 (`0x0EB`, `HACK_SMART_STARTPOS`) |
| Static label | map entry 66, store `0x1800641B8` |
| Registration | `0x1800B2DB4` (`notes/hook-registrations.md`) |
| Callback | `FUN_180183E30` |
| Installer | `0x1800AB850` (shared with the coin and practice hooks) |
| Target | `PlayLayer::resetLevel`, RVA `0x3B8EB0`, name string `0x180545515` |
| Detour | `0x180183EA0`, 0x20 bytes, install order `0xFFFFFBF8` |

### Hook

`FUN_180183E30` follows the standard shape: enable byte from `[rdx]`, settings context through
`0x180108960`, `cmp bl,1` (disable branch `0x180183E89`), module base through `0x18010A250`,
RVA resolved by `0x18010A370`, then the installer with `r8` = detour, `r9` = name,
`[rsp+0x20] = 0xFFFFFBF8` and `[rsp+0x28] = 1`.

The detour itself only calls the original and continues into the mod body, so the vanilla reset
runs first:

```
0x180183EA0  push rsi; sub rsp,0x20; mov rsi,rcx
0x180183EA8  call 0x1801A4300        ; delayed bind -> host RVA 0x3B8EB0
0x180183EAD  mov rcx,rsi; add rsp,0x20; pop rsi
0x180183EB5  jmp 0x180183EC0         ; mod body
```

`0x1801A4300` resolves `base + 0x3B8EB0` at `0x1801A4364` and caches it at `0x18080FE48`
(guard dword `0x18080FE50`, builder `0x1801A33C0`, base from `0x18010DDC0`).

### Replacement body `0x180183EC0..0x1801842CD`

No `.pdata` record; the body ends in `ret` at `0x1801842CD` after the `/GS` cookie check against
`0x1807F7040`. It is skipped entirely unless both gates hold:

- `[layer+0x3188] != 0` (`cmp qword ptr [rcx+0x3188],0` / `je 0x180184299`);
- `CCArray::count([layer+0x3888]) == 0` — the same "no checkpoint yet" test the Jumpscare gate
  uses.

It then reads four stored object lists and picks one object from each:

```
for group in (2, 3, 4, 5):
    pair = helper_0x18015AFD0(layer, group)      ; {first, count} view of a CCArray
    best = null; bestX = x_of(layer, player1)
    for obj in pair:
        x = 0x1802044B0(layer, obj, 0)
        if x > bestX: continue                   ; obj is ahead of the player
        if best != null and x < 0: continue      ; ignore negative-x objects once one is chosen
        best = obj; bestX = x
```

`0x1802044B0(layer, object, bool)` reads a double from `[object vtable+0xC8]` and passes it
through `[layer vtable+0x4B8]`; the same call with `[layer+0xDA0]` (player 1) produces the
reference coordinate, so the comparison is "nearest object at or before the player".

The four selected objects are then applied under separate runtime-assigned tags:

| Tag | Object list | Effect | Call |
| --- | --- | --- | --- |
| 237 (`0xED`) | group 2 | gamemode of the object | `0x1801842E0(layer, obj)` |
| 238 (`0xEE`) | group 3 | speed | `0x1801AA600(player, float, 0)` (host RVA `0x3A0D50`) |
| 239 (`0xEF`) | group 4 | mini/size | `0x1801A9760(player, bool, 0)` (host RVA `0x3A0700`) |
| 240 (`0xF0`) | group 5 | mirror | `0x180116CA0(layer, bool, 1)` (host RVA `0x2467D0`) |

Speed path: a selected object with `[obj+0x40C]` in `0xC8..0xCB` jumps through the table at
`0x180578BE0` to the four portal constants, and id `0x536` maps to `0x180572A9C`; with no object
selected the code reads the level's current speed index from `[layer+0xDB0]+0x14C` and indexes
the five-float table at `0x180578BC0`.

| Constant | Value | Used for |
| --- | --- | --- |
| `0x180572B00` | 0.7 | slow portal (id 0xC8) |
| `0x18054CCA4` | 0.9 | normal portal (id 0xCB) |
| `0x180572AD4` | 1.1 | fast portal (id 0xC9) |
| `0x180572B14` | 1.3 | faster portal (id 0xCA) |
| `0x180572A9C` | 1.6 | fastest (id 0x536) |
| `0x180578BC0` | 0.9, 0.7, 1.1, 1.3, 1.6 | fallback by current speed index |

Size path: `[obj+0x3A0] == 0x12` means mini on, `0x11` means off; otherwise the flag comes from
`byte [layer+0xDB0+0x150]`. Mirror path: object id `0xE` means flipped, `0xF` means not; both
players' speed and size setters are called (the second player at `[layer+0xDA8]`), while the
mirror call passes the layer.

Gamemode path `0x1801842E0`: switches on `[obj+0x3A0] - 5` over 0x25 entries (table
`0x180578BF0`), calls `0x180129B30(layer, player, obj, id, 1)` for both players where the jump
table routes there, then `0x180116E40(layer, player1, id, 0, 1)`.

### The shared object dictionary

`0x18015AFD0(layer, out_pair, group)` builds the lists the body consumes:

- `CCNode::getUserObject("absolllute.megahack")` on the layer (import hint/name entry
  `0x180645526`, `CCNode::getUserObject`); if absent it creates a `CCDictionary` and stores it
  with `CCNode::setUserObject` (hint/name `0x180645B3E`);
- `CCDictionary::objectForKey(group)` (integer key), `retain`, then `[obj+0x38]` data pointer and
  `CCArray::count` produce the `{first, count}` pair, `release`;
- an empty dictionary returns `{0, 0}`.

Only five call sites read or write that user object, so the writer side is small, but it is not
read here: `0x18015AB56`/`0x18015AEDB` are the writer path and `0x18015AB10` sorts each stored
list. `0x18015AFD0` is also called from the StartPos Switcher family (`0x18018573E`,
`0x180185813`, `0x180185933`) and from `0x180175298`, so the dictionary is shared infrastructure,
not Smart StartPos alone.

UNVERIFIED: the names of `[layer+0x3188]`, `[layer+0x3888]`, `[layer+0xDB0]` and its `+0x14C`/
`+0x150` fields (only the guards and reads above are measured); the exact coordinate returned by
`0x1802044B0` and the vtable slot identities it uses; which site populates groups 2..5 and when;
the binding between tag numbers 237..240 and the four `LEVEL/SMART_STARTPOS/DROPDOWN/*` key
names — the order matches the code and the changelog, but no site reads a key string next to a
tag number, and the fifth key (`LEGACY_CALCULATION`) has no located tag read. The v7.1 editor
row is unmapped because the only hook in this path is on `PlayLayer::resetLevel`.

## Evidence — Force StartPos Reset Camera (2026-10-26)

| Field | Value |
| --- | --- |
| Tag | 426 (`0x1AA`, `HACK_STARTPOS_RESET_CAMERA`) |
| Static label | map entry 69, store `0x1800642F3` |
| Settings key | `COSMETIC/HACK_STARTPOS_RESET_CAMERA` (`notes/settings-contents.md` line 272) |
| Registration | `0x1800B1A7D` |
| Callback | `FUN_180172B90` |
| Installer | `0x180172C30` |
| Target | `GJBaseGameLayer::loadStartPosObject`, RVA `0x235760` |
| Detour | `0x180172C00`, body `0x180172C00..0x180172C2F` (0x30 bytes, `ret` at `0x180172C2F`, code cave) |
| Name string | `0x180575E9F` "GJBaseGameLayer::loadStartPosObj" |

Callback `0x180172B90..0x180172BE8`: `movzx ebx,byte [rdx]` (enable flag) -> `call 0x180108960`
(context) -> `cmp bl,1`; on 1 it takes the module base from `0x18010A250`, resolves RVA
`0x235760` through `0x18010A370`, sets `[rsp+0x28]=1`, `[rsp+0x20]=0xFFFFFFE0`, `r8` = the
detour, `r9` = the name string, and calls the installer. The disable branch at `0x180172BE9`
passes the same detour to `0x180108A80` and returns. The tag is therefore install/uninstall,
not a per-call read.

The detour is a **post-hook** on the start-position loader:

```
0x180172C00  push rsi; sub rsp,0x20
0x180172C05  mov rsi,rcx                       ; this = GJBaseGameLayer
0x180172C08  call 0x180116150                  ; cached original loadStartPosObject
0x180172C0D  cmp qword [rsi+0x3188],0          ; is a start position active?
0x180172C15  je  0x180172C29                   ; no -> plain return
0x180172C17  mov rcx,rsi
0x180172C1A  mov dl,1
0x180172C1C  mov r8b,1
0x180172C1F  add rsp,0x20; pop rsi
0x180172C24  jmp 0x180116630                   ; host routine, (layer, true, true)
0x180172C29  nop; add rsp,0x20; pop rsi; ret
```

Both branches are 0x30 bytes apart with no fall-through, and the `je` target is the plain
return, so the forced call happens only when the loader left a start position behind.
`[this+0x3188]` is the same start-position pointer the StartPos Switcher family clears
(`[layer+0x3188]=0`) and tests (`[layer+0x3230]=(startpos!=0)`) on reset, which corroborates the
reading that this is the "start position exists" test rather than an unrelated field.

Wrapper identities, from `recovered/functions/_wrappers.tsv`:

| Thunk | Resolver | Host RVA | Note |
| --- | --- | --- | --- |
| `0x180116150` | `FUN_180114460` | `0x235760` | matches `GJBaseGameLayer::loadStartPosObject` in `_hook_sites.tsv` |
| `0x180116630` | `FUN_180116700` | `0x23FDE0` | no name in any tracked table |

The second row is the whole uncertainty. `0x23FDE0` has no name string in the image, is absent
from `_hook_sites.tsv` and `_host_methods.tsv`, and no other hook or thunk refers to it, so the
only support for calling it "reset camera" is the changelog wording plus the fact that the call
is forced immediately after a start position is loaded. Its two boolean arguments are both 1.

Method caution worth keeping: the `contains` column of `_wrappers.tsv` resolves the host RVA
against the **mod** image, so `0x23FDE0` shows as `18023fdc0_FUN_18023fdc0+0x20` even though
that mod address is an unrelated destructor stub. The column must not be read as a host-symbol
attribution.

UNVERIFIED: the identity of host RVA `0x23FDE0` (camera reset, camera snap, or something else
the loader triggers); what its two `1` arguments select; whether the call is a one-shot or
persists; and whether the vanilla loader already performs the same call under other conditions.

## Evidence — Main Level Bypass (2026-10-26)

Tag 39 (`0x027`, `HACK_MAIN_LEVELS`, static map entry 30, store `0x1800634C6`) installs
three hooks from one callback, and all three are tiny code caves around a single global
flag. The whole mechanism is scoped: two of the detours raise the flag only for the
duration of a vanilla call, and the third answers a statistic read while it is raised.

| Field | Value |
| --- | --- |
| Registration | `0x1800AFCB1` |
| Callback | `FUN_18015C9E0` (body `0x18015C9E0..0x18015CAF5`) |
| Hook 1 | `LevelPage::onPlay`, RVA `0x31D880`, detour `0x18015CB10`, installer `0x18015CB80`, name `0x180575144` |
| Hook 2 | `LevelPage::updateDynamicPage`, RVA `0x31A8B0`, detour `0x18015CB30`, installer `0x18015CD60`, name `0x180575156` |
| Hook 3 | `GameStatsManager::getStat`, RVA `0x1D51B0`, detour `0x18015CB50`, installer `0x18015CF40`, name `0x180575173` |

Each install site passes `[rsp+0x28]=1` and `[rsp+0x20]=0xFFFFFFE0`; each disable site
(`0x18015CA41`, `0x18015CA9D`, `0x18015CAF6` region) calls `0x180108A80` with the same
detour address.

### Wrappers

```
0x18015CB10  sub rsp,0x28
0x18015CB14  mov byte [rip+0x6AF55D],1        ; -> 0x18080C078
0x18015CB1B  call 0x1801A1440                 ; cached original LevelPage::onPlay
0x18015CB20  mov byte [rip+0x6AF551],0
0x18015CB27  add rsp,0x28; ret

0x18015CB30  sub rsp,0x28
0x18015CB34  mov byte [rip+0x6AF53D],1
0x18015CB3B  call 0x1801A1510                 ; cached original updateDynamicPage
0x18015CB40  mov byte [rip+0x6AF531],0
0x18015CB47  add rsp,0x28; ret
```

Both gateways are delayed-bind trampolines: `0x1801A1440` contains
`lea rcx,[rax+0x31D880]` at `0x1801A14B1`, `0x1801A1510` contains
`lea rcx,[rax+0x31A8B0]` at `0x1801A1581`. So the flag is 1 exactly while the vanilla
`LevelPage::onPlay` / `updateDynamicPage` runs.

### The statistic override

```
0x18015CB50  cmp byte [0x18080C078],1
0x18015CB57  jne 0x18014B4D0                  ; cached original GameStatsManager::getStat
0x18015CB5D  cmp byte [rdx],0x38              ; stat key 56
0x18015CB60  jne 0x18014B4D0
0x18015CB66  cmp byte [rdx+1],0               ; sub-key 0
0x18015CB6A  jne 0x18014B4D0
0x18015CB70  mov eax,0x3E7                    ; 999
0x18015CB75  ret
```

`0x18014B4D0` is the delayed-bind trampoline for RVA `0x1D51B0`
(`lea rcx,[rax+0x1D51B0]` at `0x18014B541`), so every other statistic is forwarded
unchanged. A scan of the whole `.text` for references to `0x18080C078` returns exactly five
sites — the three writers above and this one reader — so the flag has no other effect.

The stat argument is read as two bytes (`[rdx]`, `[rdx+1]`), i.e. `getStat(key, subKey)`;
only `(0x38, 0)` is overridden, and only during a level-page entry or refresh.

UNVERIFIED: what stat `(0x38, 0)` means in GD's statistic enum (the value 999 is consistent
with a completion/unlock counter but no name string or enum table in this image identifies
it); why both `onPlay` and `updateDynamicPage` need the override; and whether the vanilla
`getStat` also feeds the page's "locked" decoration through another field, which would
explain why the page still needs the real values elsewhere.

## Evidence — Wave Trail on Death and the trail-preservation hooks (2026-10-26)

`HACK_WAVE_TRAIL_ON_DEATH` (tag 419, `0x1A3`, static map entry 84, store `0x1800648BF`,
settings key `COSMETIC/HACK_WAVE_TRAIL_ON_DEATH`) has **no row** in the `mov edx,<tag>`
registration table. It is bound through a second API that composes tags into a condition and
passes a type-erased action, which is why a label-to-registration join misses it.

| Field | Value |
| --- | --- |
| Condition registration | `0x180173A30..0x180173B94`, reached from the static-initialiser stub `0x180173A20` (`lea rcx,[0x18080C518]; jmp 0x180106DC0`) |
| Condition leaves | `mov edx,0x18F` at `0x180173A7E` -> tag 399 `HACK_NO_DEATH_EFFECT`; `mov edx,0x1A3` at `0x180173A96` -> tag 419 |
| Leaf builder | `0x1801083F0(out, tag)` — kind byte at `+0x18` becomes 1, payload at `+0` becomes the tag |
| Composed node | `0x180107CE0(out, node419, 1, node399)` at `0x180173ABF`; the node keeps both children at `+8` and `+0x10`, and the fourth argument is the operator value 1 |
| Wrappers | `0x1801084E0` at `0x180173AD6`, `0x180108610` at `0x180173AE7` |
| Registration call | `0x1800915B0(ctx, &out, &action)` at `0x180173AFA`, `ctx` = `[0x18080C518]` |
| Action | `{vtable 0x180575478, body 0x180173D00}` built at `0x180173A55`/`0x180173A60` |

`0x180575478` holds `0x180163B90` / `0x180163BB0` (both `.text`); `0x180163B90` writes
`&0x180575478` through its first argument, so the pair is an interface object and
`0x180173D00` is its implementation. UNVERIFIED: the exact vtable interface and which method
invokes the body.

### The action body

`0x180173D00` takes the enable flag in `ecx` (`mov ebx,ecx; test bl,bl; je 0x180173D9E`).
When enabled it installs two hooks; the byte at `0x180173D12` is the branch taken when the
condition is off.

| Target | RVA | Detour | Installer | Name string |
| --- | --- | --- | --- | --- |
| `PlayerObject::playerDestroyed` | `0x397D40` | `0x180173E30` | `0x180167B20` | `0x180575FF8` |
| `PlayerObject::fadeOutStreak2` | `0x3A0190` | `0x180173E50` | `0x1800AAC20` | `0x180576016` |

Both installs pass `[rsp+0x20]=0xFFFFFFE0`, `[rsp+0x28]=1` and resolve the host RVA through
`0x18010A250` -> `0x18010A370`, the same delayed-bind idiom the tagged callbacks use. The
disable branch `0x180173D9E..0x180173DC2` calls `0x180108A80(ctx, detour)` once per detour —
the same unhook helper the tagged callbacks call from their own disable branches.

### The detour bodies

```
0x180173E30  48 83 EC 28               sub  rsp,0x28
0x180173E34  C6 05 FD 86 69 00 01      mov  byte [0x18080C538],1
0x180173E3B  E8 D0 E1 02 00            call 0x1801A8010        ; playerDestroyed
0x180173E40  C6 05 F1 86 69 00 00      mov  byte [0x18080C538],0
0x180173E47  48 83 C4 28               add  rsp,0x28
0x180173E4B  C3                        ret
```

`0x1801A8010` is the delayed-bind trampoline for RVA `0x397D40`: it calls `0x18010DDC0` for
the module base, adds `0x397D40`, resolves and caches in `0x180810608`, and preserves the
second argument in `ebx`, so the original `playerDestroyed` runs with unmodified arguments.
The detour only brackets that call with the flag at `0x18080C538` (a zero-fill `.bss` byte,
not part of the writable block at `0x18081BFC0`).

```
0x180173E50  80 3D E1 86 69 00 00      cmp  byte [0x18080C538],0
0x180173E57  0F 84 43 38 03 00         je   0x1801A76A0          ; fadeOutStreak2
0x180173E5D  C3                        ret
```

`0x1801A76A0` is the delayed-bind trampoline for RVA `0x3A0190` (`lea rcx,[base+0x3a0190]`,
cache `0x1808104F8`), so with the flag clear the original `fadeOutStreak2` runs unchanged.

Net effect: while `playerDestroyed` executes, the trail's fade-out is skipped, so the streak
survives the death frame. That is the wave-trail-on-death behaviour, and it is the only
death-path trail mechanism in the image:

- `PlayerObject::fadeOutStreak2` has exactly one hook site (`registrar_va 0x180173D00`).
- `CCMotionStreak::stopStroke`, `initWithFade` and `draw` belong to No Trail (tag 395,
  `FUN_180171DB0`), not to this path.

### What this binds, and what stays open

- The v8.0.3-alpha.1 row "Added Wave Trail on Death" is bound on three independent angles:
  the static label `HACK_WAVE_TRAIL_ON_DEATH`, the settings key
  `COSMETIC/HACK_WAVE_TRAIL_ON_DEATH`, and a registration whose condition names tag 419 and
  whose action installs the death-path trail hooks. The row is `mapped`.
- The v7.1 (Full) row "Added Keep Wave Trail on Death mod" describes the same behaviour, and
  only one such mechanism exists in the image. Either the v7.1 toggle was carried into v8
  under the new name or the two rows are duplicates; the image cannot separate them, so that
  row stays `cluster` with this anchor as its reason.
- UNVERIFIED: the polarity of the composed condition. The builder copies both children
  without negating either, but the evaluator that interprets the node was not read, so
  whether the action runs on `A and B` or `A or B` is open. Tag 399 is an input either way,
  which is itself new: the trail hooks are not switched by tag 419 alone.
- UNVERIFIED: no runtime observation. Nothing here was confirmed in a live game; the claim is
  static, from the detour bodies.
- Related but distinct: tag 448 (`0x1C0`, registration `0x1800B1B6B`, callback `0x1801730B0`)
  resolves `cocos2d::CCMotionStreak::stopStroke` by mangled symbol name
  (`?stopStroke@CCMotionStreak@cocos2d@@QEAAXXZ`, assembled on the stack from the immediates
  at `0x1801730F9`-`0x180173158`) and installs the label detour `0x1801732A0`, which is
  `jmp 0x1805331E0`. That is a separate trail control; no changelog row is bound to it.

## Evidence - Ball Rotation Bug (2026-10-26)

The v9.0.0 bullet "Added Ball Rotation Bug" is the only changelog row about ball rotation,
`COSMETIC/HACK_BALL_ROTATION_BUG` is the only ball-rotation settings key in
[settings-contents.md](settings-contents.md), and the image contains exactly one mechanism that
animates a player rotation. That mechanism is tag 423.

| Field | Value |
| --- | --- |
| Tag | 423 (`0x1A7`), no static label (runtime-assigned; Map B) |
| Registration | `0x1800B07E5`, bytes `ba a7 01 00 00` |
| Callback | `FUN_180165890`, body `0x180165890..0x180165A01`, `ret` at `0x180165A01` |
| Disable branch | `0x180165A02` -> `0x180108A80`, one per installed detour |
| Arg slots | `[rsp+0x20]=0xFFFFFFE0`, `[rsp+0x28]=1` for all four |

Four detours, each named by the `r9` string the callback passes and each resolved through the
same delayed-bind idiom (`0x18010A250` base -> `0x18010A370` by RVA):

| RVA | Host method | Name string | Detour | Installer | Detour does |
| --- | --- | --- | --- | --- | --- |
| `0x39BE30` | `PlayerObject::switchedToMode` | `0x1805756D8` | `0x180165A20` | `0x180165BE0` | runs the original through trampoline `0x1801A8F90` (`lea rcx,[rax+0x39BE30]` at `0x1801A8FFF`), then may add a rotation animation |
| `0x398C00` | `PlayerObject::ringJump` | `0x1805756F5` | `0x180165B60` | `0x180165DC0` | sets `byte [0x18080C228]=1`, calls the original, clears it |
| `0x38D480` | `PlayerObject::runBallRotation2` | `0x18057570C` | `0x180165B80` | `0x1800AB230` | calls the original, then records a timestamp while the flag is set |
| `0x39BF30` | `PlayerObject::hitGround` | `0x18057572B` | `0x180165BC0` | `0x180166010` | calls the original, then clears the guard |

### The state machine

Three `.bss` bytes/quadword carry the state, and a whole-`.text` scan shows they are used
nowhere else:

| Global | References | Meaning as read here |
| --- | --- | --- |
| `0x18080C228` | 3 - writes at `0x180165B64`, `0x180165B70`, compare at `0x180165B89` | "a ring jump is running" |
| `0x18080C230` | 2 - write `0x180165BA7`, read `0x180165A73` | `_Xtime_get_ticks()` of the last ball rotation |
| `0x18080C238` | 4 - test `0x180165A65`, compare `0x180165B97`, write `0x180165BA0`, write `0x180165BC9` | "a ball rotation has happened since the last ground contact" |

`0x1805318E0` is `_Xtime_get_ticks`, so the comparison at `0x180165A7A` against `0x98BD8F`
(10,009,999 ticks of 100 ns) is a one-second window.

Sequence: a `ringJump` raises `0x18080C228` for the duration of the call, so only the
`runBallRotation2` calls made *from inside* a ring jump record `0x18080C230` and raise
`0x18080C238`. `hitGround` clears `0x18080C238`. A later `switchedToMode` therefore sees
"rotated during a ring jump, has not touched the ground since".

### The rotation animation

`switchedToMode` detour entry checks, in order (`0x180165A34..0x180165A80`):

1. `edx` (the new mode) must be `<= 0x29` and must have its bit set in `0x20000080020`, i.e.
   only modes 5, 19 and 41 are handled.
2. `byte [player+0x9BB]` is sampled into `ebx` before the original runs.
3. After the original returns: `edi != 0x10`, `bl != 0` and `byte [0x18080C238] & 1` must all
   hold, and `_Xtime_get_ticks() - [0x18080C230]` must be `<= 0x98BD8F` (a rotation within the
   last second). The "not handled" path tail-jumps the trampoline, so the original always runs.

When all of that holds the detour builds the animation at `0x180165AAD`:

| Site | Value |
| --- | --- |
| `0x180165AB2` | `ucomiss 0, [player+0x720]`, `seta al` - al = 1 when `[player+0x720] < 0` |
| `0x180165AC3` | `xmm6 = {1.0f, -1.0f}[al]` from the table at `0x180549BD0` (`00 00 80 3f`, `00 00 80 bf`) |
| `0x180165AD0` | `xmm7 = 270.0f * xmm6` (constant `0x180572B08` = `00 00 87 43`) |
| `0x180165AE0` | `[player+0x558]->stopActionByTag(0x1E6813A)` |
| `0x180165AEC` | `xmm6 = xmm6 * 90.0f` (constant `0x180572A8C` = `00 00 b4 42`) |
| `0x180165AFA` | vtable slot `+0x150` on `[player+0x558]` called with `xmm6` - snap the node rotation to +/-90 degrees |
| `0x180165B0B` | `CCRotateBy::create(0.5f, xmm7)` (duration `0x180534410` = `00 00 00 3f`) |
| `0x180165B1B` | `CCEaseOut::create(action, 1.2f)` (rate `0x1805756D4` = `9a 99 99 3f`) |
| `0x180165B2E` | vtable slot `+0x40` on the action called with `0x1E6813A` - set the same tag so a repeat can cancel it |
| `0x180165B4C` | tail `jmp 0x180532CE0` = `[player+0x558]->runAction(action)` |

So the detour does not reimplement ball rotation: it normalises it. When the player switches
into a ball-family mode within a second of a ball rotation that happened during a ring jump, the
node at `[player+0x558]` is snapped to +/-90 degrees in the direction of the angular velocity in
`[player+0x720]`, and a 0.5 s, 1.2-rate ease-out rotation of +/-270 degrees carries it to a clean
multiple of 360. A ground contact re-arms the whole test.

`[player+0x720]` has five memory references in the image, four of them host code
(`0x18020666B`, `0x180207717`, `0x180208A54`, `0x180208A5C`) that move it as a float between
objects - consistent with a rotation/angular-velocity field, which is what the sign test needs.

### What stays open

UNVERIFIED: that `[player+0x558]` is the ball sprite node rather than another child node - the
detour proves only that it is a `cocos2d::CCNode` with a float setter in vtable slot `+0x150`
and a `setTag` in slot `+0x40`. UNVERIFIED: which modes 5, 19 and 41 are in the enumeration
`PlayerObject::switchedToMode` receives, and what `byte [player+0x9BB]` holds; the mask is
recorded as measured, not named. UNVERIFIED: the direction of the feature - the detour adds a
correction animation, which reads as a fix for the bug the row is named after, but no runtime
observation or changelog wording settles whether the toggle restores vanilla behaviour or
applies the fix. UNVERIFIED: the JSON path that writes tag 423 (settings keys live in the
profile, not in the image).

## Evidence - tag 438 and the coin post-pass on collisionCheckObjects (2026-10-26)

Tag 438 (`0x1B6`) is a second detour on `GJBaseGameLayer::collisionCheckObjects` (RVA
`0x214960`, name `0x1805757D9`), the routine already hooked by Collect Coins in Practice (tag
297) and All Modes Platformer (tag 241). Registration `0x1800B09C1` (`ba b6 01 00 00`),
callback `FUN_180167210` (`0x180167210..0x180167268`, no post-install work), detour
`0x180167280`, installer `0x180167330` with `[rsp+0x20]=0xFFFFFFE0`, `[rsp+0x28]=1`.
Registration and detour re-checked 2026-10-26 with `scripts/tag_callback_map.py` and
`scripts/callback_hooks.py`; body listed with capstone over `_original.dll`.

```
0x180167287  mov   edi, r9d              ; object count
0x18016728A  mov   rsi, r8               ; object vector
0x18016728D  movss xmm0, [rsp+0x70]
0x180167293  movss [rsp+0x20], xmm0      ; forward the stacked argument
0x180167299  call  0x180115840           ; original (delayed-bind thunk)
0x18016729E  test  edi, edi
0x1801672A0  jle   0x1801672F3           ; nothing to post-process
0x1801672B8  mov   rax, [rsi]            ; begin
0x1801672BB  mov   rcx, [rsi+8]          ; end
0x1801672C2  sar   rcx, 3
0x1801672C6  cmp   rcx, rbx
0x1801672C9  jbe   0x1801672FB           ; bounds check
0x1801672CB  mov   rcx, [rax+rbx*8]
0x1801672CF  mov   eax, [rcx+0x3A0]      ; object id
0x1801672D5  cmp   eax, 0x1F
0x1801672D8  je    0x1801672DF
0x1801672DA  cmp   eax, 0x16
0x1801672DD  jne   0x1801672B0           ; next object
0x1801672DF  cmp   byte [rcx+0x3D2], 1
0x1801672E6  jne   0x1801672B0           ; next unless the flag is set
0x1801672EB  call  [rax+0x460]           ; one per-object call
```

Object ids `0x1F`/`0x16` with a flag at `+0x3D2` are the same two fields the Collect Coins in
Practice note identifies as the coin pair (there the detour keeps entries whose `+0x3D2` is
clear), so this pass runs over the coins the collision check just marked and calls slot
`+0x460` on each once.

The only v9.0.8 row about coins is "Coin Shower", and tag 438 sits one below tag 439 (No End
Shake, mapped to v9.0.8), which fits that release group.

UNVERIFIED: what slot `+0x460` does (a pickup or particle effect is the plausible reading and
was not observed); whether `+0x3D2 == 1` means collected; and the row binding, since no
settings key or label in the image names this mechanism.
