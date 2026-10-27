# Geometry Dash 2.2081 save container: recovered and implemented

- **Question** — the level fixture the runtime-validation manifest names
  (`fixtures/runtime/safe-mode-smoke.gmd`) does not exist, and `notes/runtime-validation.md`
  recorded the format as UNVERIFIED. What is a GD 2.2081 save, and can a script write one?
- **Method** — read the files GD itself wrote on this machine (read-only, never committed),
  derive the container from a known-plaintext guess, then confirm on a second file written by a
  different code path, then implement decode **and** encode and test them.
- **Date** — 2026-10-27.
- **Tool** — `harness/tools/gd_save.py` (+ `test_gd_save.py`, 12 cases).

## The container

    file = XOR( base64url_nopad( gzip( plist_xml ) ), 0x0B )

and a level's object string inside the plist:

    k4 = base64url_nopad( gzip( "<object;object;...>" ) )

Both are the same construction: gzip, url-safe base64 without padding, and — for the outer file
only — a single-byte XOR of `0x0B`.

### How it was derived (and why it is not a guess)

1. `CCLocalLevels.dat` starts `43 3f 78 42 4a 4a 4a 4a ...`, which is printable but not an
   alphabet: the byte set is 66 symbols over `0x0B..0x7F` and the entropy is 6.0 bits — a
   64-symbol encoding, not text and not a stream cipher.
2. The smallest byte present in the file is `0x0B`, and `0x43 ^ 0x0B = 'H'`, `0x3f ^ 0x0B = '4'`,
   `0x78 ^ 0x0B = 's'`, `0x42 ^ 0x0B = 'I'`: the plaintext begins `H4sI`, the standard base64
   prefix of a gzip stream with a zero timestamp. One hypothesis explains the alphabet size, the
   minimum byte and the first four bytes at once.
3. Decoding needs the **urlsafe** alphabet: after the XOR the payload contains `-` and `_` and
   no `+`, `/` or padding. With `+`/`/` it fails; with `-`/`_` it decodes.
4. The decompressed bytes are GD's own plist dialect, `<?xml version="1.0"?><plist
   version="1.0" gjver="2.0">` with `<k>/<d>/<s>/<i>/<r>/<t />` elements — not Apple's plist,
   which is why a stock plist parser is the wrong tool.

### Second angle: the same container, different code path

| File | gzip header | XML bytes | XML SHA-256 (prefix) |
| --- | --- | --- | --- |
| `CCLocalLevels.dat` | `1f 8b 08 00 00000000 00 0a` | 3,086,667 | `7ef8e7b50a396c40` |
| `CCGameManager2.dat` | `1f 8b 08 00 00000000 00 0a` | 5,377,449 | `4e1ecf3c653212f1` |

Two different save kinds, same header, same procedure, both producing a well-formed plist with
a plausible root (`LLM_01` for levels, `valueKeeper` for the game manager).

`k4` was confirmed the same way: base64url-decoding it yields a gzip header
(`1f 8b 08 00 00000000 00 0a`) and 703,713 bytes of object string beginning
`kS38,1_0_2_0_3_0_11_255...` — the colour-channel block that precedes a level's objects.

### Encoding is deterministic

`encode()` writes GD's own header fields (zero mtime, XFL 0, OS 10), `zlib` level 6, and
url-safe base64 without padding. Same XML in, same container out — that is what makes a
generated fixture usable as evidence. Note that it is *not* byte-identical to a file GD wrote
from the same XML: the deflate stream depends on the compressor. Only decode has to match GD.

## A finding this exposed, which matters more than the format

**`%LOCALAPPDATA%\GeometryDash` is not isolated per Geode profile.** The `a` profile (Steam) and
`runtime511` both resolve GD's save directory to the same place: the capture runs at 13:51
rewrote `CCLocalLevels.dat` and `CCGameManager.dat` in the user's live save, and there is one
copy of each file, not one per profile.

So `profile.isolation: disposable` in `harness/scenarios/safe-mode-smoke.json` is **not
currently true for save data**; `runtime511` isolates the game binaries and Geode's own mod
storage, not GD's saves. That contradicts an explicit gate row ("a run against a live profile is
not eligible"), and it is the reason installing a generated level fixture by writing
`CCLocalLevels.dat` in place would destroy the user's levels.

**Redirecting `%LOCALAPPDATA%` for the child process will not work, and that was checked before
it cost a run.** Import tables of the three binaries in `GD-Runtime-511`:

| Module | Path-related imports |
| --- | --- |
| `GeometryDash.exe` | no environment, known-folder or shell-folder API at all |
| `libcocos2d.dll` | `SHELL32!SHGetFolderPathA`; **no** `GetEnvironmentVariable*`, no `getenv` |
| `Geode.dll` | `SHGetFolderPathW`, `ExpandEnvironmentStringsA`, `GetEnvironmentVariableA/W`, `getenv`, `getenv_s` |

The writable path is built by cocos2d, and cocos2d resolves it through `SHGetFolderPathA`
(CSIDL_LOCAL_APPDATA), which reads the shell known folder from the profile, not the environment.
Two angles: the import is present, and no environment-reading API is imported by the module that
owns the path — while the directory GD actually writes *is* the LOCAL_APPDATA known folder.
Geode does read environment variables, but for its own directories, which is exactly the split
observed earlier (mod data under `%LOCALAPPDATA%\GeometryDash\geode`, GD's saves beside it).

Closing it therefore needs one of:

- a second Windows user account (its known folders are separate), 
- a VM or Windows Sandbox,
- or a scripted backup/install/restore of `%LOCALAPPDATA%\GeometryDash\*.dat` around each run,
  which is *controlled mutation of the live save*, not isolation, and must be labelled as such.

The third is implemented: `harness/tools/save_sandbox.py` (+ `test_save_sandbox.py`, 29 cases)
with `--backup`, `--install`, `--status` and `--restore`. Its invariants are the point:

- it refuses every operation while `GeometryDash.exe` is running (process-image check, once at
  the start of the operation; a guard, not a lock);
- it refuses to install unless a digest-verified backup exists, and refuses to overwrite a save
  whose bytes have changed since that backup;
- every write is atomic per file (temp file beside the target, fsync, rename), so a refusal or a
  crash leaves the previous file intact;
- an install reads back what it wrote and decodes it, and rolls back to the backed-up bytes if
  the bytes do not verify;
- a restore reports exactly which files were restored, which failed and which were never
  touched, and re-running it after the cause is fixed finishes the job;
- `--status` distinguishes `same`, `differs` and `missing`, and lists files that appeared after
  the backup as *extras*.

**Extras are reported, never deleted.** A file that was not in the backup is left in place: GD
may have written it, and it may be the only copy of something. Identify it before relaunching.

Its own docstring says it is not isolation.

Until one is in place, a fixture must not be installed into the live save directory, and an M3
run is not eligible on the isolation row regardless of how good the comparison is.

## Runbook for the fixture (needs a run)

Nothing below installs anything until the operator has a backup:

```
python harness\tools\save_sandbox.py --backup  --save-dir "%LOCALAPPDATA%\GeometryDash" --backup-dir <outside-the-repo>\gd-save-backup
python harness\tools\save_sandbox.py --install --save-dir "%LOCALAPPDATA%\GeometryDash" --backup-dir <outside-the-repo>\gd-save-backup --fixture harness\fixtures\runtime\safe-mode-smoke.gmd
python harness\tools\save_sandbox.py --status  --save-dir "%LOCALAPPDATA%\GeometryDash" --backup-dir <outside-the-repo>\gd-save-backup
```

Then launch the profile, play the level named `safe-mode-smoke fixture`, quit, and run:

```
python harness\tools\save_sandbox.py --restore --save-dir "%LOCALAPPDATA%\GeometryDash" --backup-dir <outside-the-repo>\gd-save-backup
```

Every step above refuses while the game is running, so quit it first — including after the run,
before the restore. If `--status` or `--restore` reports an `extra:` file, do not relaunch: the
file appeared after the backup and the tool leaves it in place rather than guessing whose it is.

`--status` between install and restore answers "has the game rewritten the save since?". If
the fixture did not load, the restore is still correct and the run simply produced no
`level_started` with the fixture's digest (the trace never records the level's name).

Three things this does **not** do: isolate the save (see above), prove GD loaded the fixture
(a run does — `level_started` records the digest of the level's object string, which
`level.sha256` must equal), or protect the user's level from being edited by the game.

## What this unblocks, and what is still missing

Unblocked: a fixture generator can now write a complete `CCLocalLevels.dat` (or a single-level
`.gmd` in the same dialect) from a declarative spec, because both layers — plist and `k4` — are
implemented and tested.

### The generator, and the fixture it writes

`harness/tools/make_level_fixture.py` (+ `test_make_level_fixture.py`, 20 cases) emits one
level from a declarative spec: three blocks in the player's path at x = 50, 100 and 150 grid
units — roughly 4.8 s, 9.6 s and 14.5 s of running at ~10 units/s — the last one also setting
the level's length. Coordinates are 30 px per unit, and the first version of this fixture used
x = 150/300/600, which put the first obstacle ~14.5 s out and made the level look empty for a
whole short session. That was a fixture defect, recorded here because it is exactly the kind of
thing a "the run passed" claim hides.

| Artifact | Detail |
| --- | --- |
| `harness/fixtures/runtime/safe-mode-smoke.gmd` | 947 bytes of GD plist XML |
| file SHA-256 | `810e276e96e988d6f3e8c82ba08563d4cca43f8de84de232844e07995ee2b5e4` |
| level-string SHA-256, decoded object string (126 bytes) | `0ecf8fd6ae443b1f51b542198f64a73bb80dc4f60deb842de9a2c550af8d6e0a` (not what the trace shows) |
| level-string SHA-256, encoded `k4` text | `58d4b320fced5935ff298ae20feefb565de405f010cdfadd86efe6713fdc33fd` ← `level.sha256` |
| level dict | the 18 keys a working local level has, in the editor's order |
| `k4` | `base64url(gzip("kS38,…;1,1,2,50,3,15;…"))`, deterministic |

`make_level_fixture.py --digests` prints the last two digests plus the file digest as JSON;
generation is deterministic, so the file digest is the regeneration identity check, not a gate
input.

The `.gmd` is the canonical text form and *our* interchange artifact, and `gd_save.py --check`
correctly refuses it, because a `.gmd` is not a container. `--backup-dir` is required to
install: the tool converts this XML into `CCLocalLevels.dat` **and** `CCLocalLevels2.dat`
through `gd_save.encode`, and refuses to write either without a digest-verified backup (see the
isolation section below).

Session state (`k18` attempts, `k36` jumps, `k80` edit seconds) and the editor camera
(`kI1`–`kI3`) are zero, which is what a level that has never been played looks like. `k101` is
the only field whose meaning is unknown; its *shape* is copied, its value is zeros.

### The `level` row is an observation (2026-10-27)

The weaker version of this row compared the manifest's hash with a copy of itself: `meta.level`
reaches the trace from the run context, which the runner copies from the manifest, so it could
show `pass` without anything having observed which level loaded. Recording a **digest** instead
of comparing the context is what closes that, and the digest is not player data.

`level_started` now records the SHA-256 of the level's object string
(`GJGameLevel::m_levelString`, the 2.2081 binding field at `GJGameLevel.hpp:487`) as
`{"state":"observed","type":"str","value":"<64 lower-case hex>"}`. Hex rather than base64
because the manifest spells digests in hex, so the comparator compares two strings with no
encoding step. `unavailable` when no level was passed or its string was empty; the level's
**name** is still never read, not even to hash it.

`harness/scenarios/safe-mode-smoke.json` therefore now pins `level.sha256` =
`58d4b320…`, the digest of the **encoded `k4` text**. That was not a guess kept in place: the
two candidate forms were pre-registered above, and the first observation (run `65d0bde5`,
release, repetition 0, 2026-10-27) recorded exactly `58d4b320…` at `level_started`. So in 2.2081
`m_levelString` holds `base64url(gzip(object string))` - the same bytes the save stores - and
not the decoded object string, whose digest `0ecf8fd6…` stays in the table above as the
candidate that lost. Two things follow, and both are deliberate:

- `level.sha256` **no longer means the `.gmd` file hash**. It means "the digest every run must
  observe at `level_started`". The file hash stays in this note and in the generator's
  `--digests` report, where it belongs.
- The pin records what the game *does*, not what would have been convenient: had the observation
  disagreed with either candidate, the pin would not have been moved to meet it without a
  finding explaining why.

The comparator's row (`harness/src/comparator.cpp`, `check_comparability`) has two outcomes,
both new:

| Observation | Result |
| --- | --- |
| any run recorded no digest (or no `level_started`) | `blocked` — "a run did not record the digest of the level it started"; absence is not agreement |
| any run's digest differs from the manifest | `invalid` — "a run started a level whose digest is not the manifest's fixture" |

The earlier `level` row (run-context hash and source must match the manifest) is unchanged and
still catches a runner that copied a different manifest; it is just no longer the only row.

Still missing before an M3 run:

- **Save isolation.** The fixture cannot be installed into a live save (see above): the
  environment redirect is ruled out by the import tables, so the run needs a separate Windows
  account, a VM, or the reversible-but-not-isolating `save_sandbox.py` under an explicit label.
- ~~Proof GD loads the generated file.~~ **Partially closed, and the earlier claim was too
  strong.** Run `e717db95-1d42-47af-b4b0-361f27eb2ae1` was a complete capture and the fixture was
  byte-verified on disk before launch, but the operator was looking at their **own** levels: the
  install had been rolled back at startup (see below), so nothing in that run shows the generated
  level loading. What *is* established by observation, on 2026-10-27:

  | Observation | Evidence |
  | --- | --- |
  | GD restores `CCLocalLevels.dat` from `CCLocalLevels2.dat` at startup | the primary flipped to the 3 MB backup 1.3 s after launch, sampled 4×/s |
  | an install of the primary alone is therefore undone | same measurement; the operator then saw their own levels again |
  | a 43-key dict with invented zeros and an empty `<d></d>` for `kI6` yields **no levels** | operator report, no trace needed |
  | a 4-key dict (`kCEK`,`k2`,`k4`,`k5`) is **listed**, but as an *online* level ("Get it") | operator report |
  | the same dict plus `k21 = 2` (Local) yields **no levels** | operator report |

  The last two contradict each other about what the client requires, and no amount of guessing
  resolved it: see [gd-docs-level-format.md](../references/gd-docs-level-format.md) for the key
  table and exactly what it does and does not settle.

  **Resolved 2026-10-27 by reading a level the editor wrote.** A level created in the editor
  (`megahacklevel`) and read back through `gd_save.decode` has **18 keys**, not 43:

  ```
  kCEK k18 k36 k2 k4 k5 k101 k13 k21 k16 k80 k50 k47 k48 kI1 kI2 kI3 kI6
  ```

  Three corrections follow, and they explain every failed attempt:

  - The earlier "43 keys" was a measurement error: that scan flattened `kI6`'s contents into the
    level dict. `kI6` is a **dict of fourteen single-character slots**, and emitting it as an
    empty `<d></d>` is what made the 43-key attempt show nothing.
  - `k21 = 2` is correct and is what the editor writes. The 4-key dict showed "Get it" because it
    had no `k21`, and the "k21 = 2 shows nothing" attempt failed for the `kI6` reason above, not
    because of `k21`.
  - `k48` is the **object count** and agrees with `k4` (2 objects in the editor's level, 2 in
    `k48`). A level whose count disagrees with its objects is a different level.

  The fixture now emits exactly that key set, in that order, with the same element types; its
  `k48` is derived from the object list rather than written by hand. `k101` is the one field
  whose meaning is unknown (a 39-character list of twenty small integers); the fixture repeats
  the shape with zeros and says so.

  **It still did not list — and that was the last missing piece.** After the client refused a
  file whose root held only `LLM_01`, it rewrote `CCLocalLevels.dat` itself at quit. The file it
  wrote is the specification:

  ```xml
  <k>LLM_01</k><d><k>_isArr</k><t /></d>   <!-- levels, empty -->
  <k>LLM_02</k><i>47</i>                   <!-- format version -->
  <k>LLM_03</k><d><k>_isArr</k><t /></d>   <!-- second array -->
  ```

  A played save has the same three keys, with the levels under `LLM_01`. A fixture with only
  `LLM_01` is parsed and then discarded: the client wrote a 160-byte empty save over a 958-byte
  file that contained two levels, which is the proof that it read the file and rejected its
  contents, not that it failed to find it.

  With `LLM_02 = 47` and an empty `LLM_03` added, the fixture **works**: on 2026-10-27 the
  operator saw `safe-mode-smoke fixture` in the levels list and the three blocks in the level.
  That closes "GD loads the generated file".
