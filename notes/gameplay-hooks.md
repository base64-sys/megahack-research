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
and their defaults; the jump-table destinations; and what `+0x516` actually controls.
Nothing was inferred about the visual result from the feature's name.

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
