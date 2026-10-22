# Safe Mode feature cluster

- **Question** — how does Mega Hack's Safe Mode / cheat safety work?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — exported API

From [hackpro-api.md](hackpro-api.md):

| VA | Demangled |
| --- | --- |
| `0x18019B350` | `mega::utils::safe_mode::areCheatsEnabled()` |
| `0x18019B450` | `mega::utils::safe_mode::disableAllCheats()` |
| `0x18019B5A0` | `mega::utils::safe_mode::hasCheatedInAttempt()` |

`hasCheatedInAttempt` is eight bytes: it returns `DAT_18080CBD8`. That global is the
attempt-level cache, not a live config query; its writers are listed below.

## Evidence — hook registration

`FUN_180199D10` (`0x180199D10`) registers three callbacks:

| GD method | RVA | Callback | Behaviour |
| --- | --: | --- | --- |
| `GJGameLevel::savePercentage` | `0x16C8B0` | `FUN_180199DF0` | veto: returns without calling the original when the gate is true |
| `PlayLayer::levelComplete` | `0x3A7A80` | `FUN_180199E30` | sets `PlayLayer+0x3230` around the original when the gate is true |
| `PlayLayer::resetLevel` | `0x3B8EB0` | `FUN_180199E80` | calls the original, then refreshes the attempt cache |

A second registration function, `FUN_1801990B0` (`0x1801990B0`), installs another
`PlayLayer::resetLevel` callback (`FUN_180199250`), the `PlayLayer::postUpdate`
accumulator (`FUN_1801992B0`), and four collision/init callbacks. So `resetLevel` has two
Mega Hack hooks: one refreshes the safe-mode cache, the other clears the progress
accumulators. Neither calls the gate.

`FUN_180199250` calls the resetLevel thunk `FUN_1801A4300`, then zeroes
`DAT_18080CB68`, `DAT_18080CB69`, `DAT_18080CB70`, `DAT_18080CB78`, the checkpoint vector
(`DAT_18080CB50`/`DAT_18080CB58`), `DAT_18080CB80` and `DAT_18080CB88`.

## Evidence — the shared gate

`FUN_18019B5F0` (`0x18019B5F0`, 117 bytes) has exactly two direct callers:
`FUN_180199DF0` and `FUN_180199E30`. Read in full:

```
if (FUN_1801ACDB0() == 0) return false;                 // no current level
if (isHackEnabled(HACK_SAFE_MODE = 482)) return true;   // tag 0x1E2
if (isHackEnabled(HACK_AUTO_SAFE_MODE = 481) &&         // tag 0x1E1
    DAT_18080CBD8 != 0) return true;                    // hasCheatedInAttempt cache
if (getConfigVar<bool>(CHECKBOX_NOCLIP_DISABLE_PROGRESS = 73) &&
    FUN_180199520() < 1.0) return true;                 // tag 0x49
return false;
```

The tag identities are from the static maps in
[static-hack-label-map.md](static-hack-label-map.md): 481/482 are the Map B `HACK_*`
entries, 73 is the Map A `CHECKBOX_*` entry. The gate reads the 21-table tag 73, not a
second Safe Mode flag.

Effects:

- `FUN_180199DF0` (`savePercentage` hook): if the gate is true, it returns before
  `FUN_1801462C0`. GD never records the percentage.
- `FUN_180199E30` (`levelComplete` hook): if the gate is true and
  `PlayLayer+0x3230 == 0`, it sets that byte to 1, calls `FUN_1801A5190` (the original),
  sets it back to 0. Otherwise it calls the original directly. The gate does **not** block
  completion; the byte is a re-entrancy marker. Which callback reads
  `PlayLayer+0x3230` is UNVERIFIED.
- `FUN_180199E80` (`resetLevel` hook): calls the original, then `FUN_18019B330`, which
  does `DAT_18080CBD8 = mega::utils::safe_mode::areCheatsEnabled()`.
- `FUN_180199250` (second `resetLevel` hook): clears the progress accumulators as above.

### The `DAT_18080CBD8` writers

| VA | Function | Write |
| --- | --- | --- |
| `0x18019B330` | `FUN_18019B330` | `= areCheatsEnabled()`, called after every `resetLevel` by `FUN_180199E80` |
| `0x18019B2F0` | `FUN_18019B2F0` | `= areCheatsEnabled()` after setting the ruleset index (`DAT_1807F9330` from `0x18057B1B0[0..2]`); registered for tag 478 (`0x1DE`) by `FUN_18019B050` |
| `0x18019B670` | `FUN_18019B670` | `|= areCheatsEnabled()`, a latch with no direct caller in the recovered tree |

`FUN_18019B5A0` (`hasCheatedInAttempt`) reads it. The cache is therefore "cheats were
enabled at this reset, or the latch fired", not a live query. `FUN_18019B5B0` also reads it
(two callers) and writes a small status output.

## Evidence — progress accounting

`FUN_1801992B0` (`PlayLayer::postUpdate`, registered by `FUN_1801990B0`) calls the
original `FUN_1801A37C0`, then, when the three in-level conditions hold
(`FUN_18015A2F0() == 0`, `PlayLayer+0x324A == 0`, `PlayLayer+0x3258 == 1`):

- adds the frame delta `param_2` to `DAT_18080CB70` (total level time);
- if `DAT_18080CB68 == 1`, adds the delta to `DAT_18080CB78` (cheat-attributed time); on
  the first frame of a cheat segment (`CB69 == 0`) it also records
  `FUN_1801A4CB0(playLayer)` in the checkpoint vector, and the first clean frame after a
  segment updates the last entry;
- sets `DAT_18080CB69 = DAT_18080CB68` and then `DAT_18080CB68 = 0`.

`FUN_180199520` returns `1.0 - DAT_18080CB78 / DAT_18080CB70` when the denominator is
positive, otherwise `1.0`. It is the **non-cheat share of level time**. The gate's third
branch fires when that share is **below 1.0** — any cheat-attributed frame — not when it
reaches 1.0. The earlier note had this comparison inverted.

No assignment of `1` to `DAT_18080CB68` appears in the recovered tree; only
`FUN_1801992B0` and `FUN_180199250` write the flag, and both write 0. The code that marks
a frame as cheat-attributed is therefore not in the recovered set (or is reached through a
pointer).

## Evidence — settings keys

From [settings-contents.md](settings-contents.md) and the static maps:

| Internal label | Tag | Map | Observed JSON key |
| --- | --: | --- | --- |
| `HACK_AUTO_SAFE_MODE` | 481 (`0x1E1`) | B | `CHEAT_SAFETY/HACK_AUTO_SAFE_MODE` (enabled in the sampled profile) |
| `HACK_SAFE_MODE` | 482 (`0x1E2`) | B | not observed; `CHEAT_SAFETY/HACK_SAFE_MODE` is absent from the sampled file |
| `HACK_SAFE_MODE_POPUP` | 483 (`0x1E3`) | B | `CHEAT_SAFETY/HACK_SAFE_MODE_POPUP` (enabled; not read by the gate) |
| `CHECKBOX_NOCLIP_DISABLE_PROGRESS` | 73 (`0x49`) | A | not observed; `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE` is a candidate |

The `CHEAT_SAFETY/*` entries are profile names, not `V_BOOL` keys; the
`LEVEL/NOCLIP/*` entry is a `V_BOOL` key. See
[settings-contents.md](settings-contents.md) for the distinction.

UNVERIFIED: the JSON path that writes tag 73. The sampled `V_BOOL` set has no
`DISABLE_PROGRESS` key; `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE` is `True` in the same profile,
but no recovered code ties that path to tag 73. The UI definition stream, not the plain
strings in `.rdata`, is where that binding must be recovered. The same caveat applies to
tag 482: `CHEAT_SAFETY/HACK_SAFE_MODE_POPUP` is tag 483, not 482.

The sampled profile is user data, not a shipped default. Defaults for 481/482/73 remain
UNVERIFIED.

## Pattern

Safe Mode is a **progress-validity gate**, not a gameplay toggle. Two hooks consult it:
`savePercentage` is vetoed, and `levelComplete` is marked with `PlayLayer+0x3230`. The
resetLevel hooks refresh the attempt cache and clear the accumulators; a separate
ruleset-change callback (`FUN_18019B2F0`) and a latch (`FUN_18019B670`) also write the
cache.

`areCheatsEnabled` (`0x18019B350`) and `disableAllCheats` (`0x18019B450`) are separate
operations. `areCheatsEnabled` enumerates the configured cheat ruleset through
`FUN_180090BC0` and returns true when that vector is non-empty, with additional true-return
paths for 0x1F6/0x1F5, 0x365 and 0x1E7/0x1E6. `disableAllCheats` enumerates the same
ruleset, calls `setHackEnabled(tag, 0)` for each entry, then clears 0x1F6 (when its 0x1F5
threshold is below 1.0), config 0x365 and hack flag 0x1E7.

`disableAllCheats` is called by `FUN_18019A780` (`0x18019A780`), a 42-byte callback that
sets `DAT_18080CBB0 = 1`, calls `disableAllCheats`, calls the function pointer at
`param_1+0x10` with `*(param_1+8)` and 0, then clears the flag. It has no direct caller in
the recovered tree.

The likely trigger is the confirmation dialog: `FUN_18019B050` registers
`FUN_18019B1B0` for tag 480 (`0x1E0`), and that callback shows
`MESSAGE/CHEAT_SAFETY/CONFIRM_DISABLE` with a lambda whose body is
`CheatSafety_hookpro::disableCheats(hackpro::Tag)`. `FUN_18019A780` has exactly that
lambda shape (captured tag at `+8`, callable at `+0x10`). UNVERIFIED: the exact vtable
binding from the message callback to `FUN_18019A780`.

## Open questions

- UNVERIFIED: what sets `DAT_18080CB68` to 1.
- UNVERIFIED: what triggers `FUN_18019B670`; it has no direct caller or registration
  reference in the recovered tree. `FUN_18019B2F0` is the tag-478 listener registered by
  `FUN_18019B050`.
- UNVERIFIED: the JSON key for tag 482, and whether `LEVEL/NOCLIP/DROPDOWN/SAFE_MODE` is
  tag 73 or a different tag.
- UNVERIFIED: which callback reads `PlayLayer+0x3230` and what it suppresses.
- UNVERIFIED: the shipped defaults for tags 481, 482 and 73.
- UNVERIFIED: which callers of `areCheatsEnabled` / `disableAllCheats` are the
  ranked-play gate.
- UNVERIFIED: whether Safe Mode state persists across levels or resets per attempt; the
  `hasCheatedInAttempt` cache is refreshed on every `resetLevel`, while the 481/482
  toggles are ordinary config values.
- UNVERIFIED: the exact vtable binding from the `MESSAGE/CHEAT_SAFETY/CONFIRM_DISABLE`
  callback to `FUN_18019A780`.
