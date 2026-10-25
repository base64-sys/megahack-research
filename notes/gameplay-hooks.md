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
