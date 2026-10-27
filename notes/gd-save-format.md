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

The third is implemented: `harness/tools/save_sandbox.py` (+ `test_save_sandbox.py`, 10 cases)
with `--backup`, `--install`, `--status` and `--restore`. Its invariants are the point — it
refuses to install unless a digest-verified backup exists, refuses to overwrite a save whose
bytes have changed since that backup, refuses a second backup over an existing one, and verifies
every restored file against its recorded digest. Its own docstring says it is not isolation.

Until one is in place, a fixture must not be installed into the live save directory, and an M3
run is not eligible on the isolation row regardless of how good the comparison is.

## What this unblocks, and what is still missing

Unblocked: a fixture generator can now write a complete `CCLocalLevels.dat` (or a single-level
`.gmd` in the same dialect) from a declarative spec, because both layers — plist and `k4` — are
implemented and tested.

### The generator, and the fixture it writes

`harness/tools/make_level_fixture.py` (+ `test_make_level_fixture.py`, 12 cases) emits one
level from a declarative spec: three blocks in the player's path, at x = 150, 300 and 600, the
last one also setting the level's length.

| Artifact | Detail |
| --- | --- |
| `harness/fixtures/runtime/safe-mode-smoke.gmd` | 1,031 bytes of GD plist XML |
| SHA-256 | `edcfd088e227efda17a4c45a635518771d2261ad99ae3799cdda4bde9bc73e34` |
| level dict | the 43 keys GD 2.2081 wrote for a real level, same order |
| `k4` | `base64url(gzip("kS38,…;1,1,2,150,3,15;…"))`, deterministic |

The `.gmd` is the canonical text form and the file whose digest the manifest will pin; it is
*our* interchange artifact, and `gd_save.py --check` correctly refuses it, because a `.gmd` is
not a container. `--install <save-dir>` converts the same XML into `CCLocalLevels.dat` through
`gd_save.encode`, refusing to replace an existing save without `--force` and keeping a `.dat.bak`
when forced.

Values for the keys whose meaning is not established (`k18`, `k36`, `k23`, `k11`, `k16`, `k21`,
`k27`, `k42`, `k45`, `k50`, `k48`, `k106`, `kI*`) are ordinary defaults. Only `kCEK`, `k2`,
`k4` and `k5` are read as what they obviously are.

### The `level` row is an assertion, not an observation

`meta.level` reaches the trace from the run context, which the runner copies from the manifest.
So the gate's level row compares the manifest's hash with itself: it can show `pass` without
anything having observed which level the game loaded.

**Half of that is now closed.** `level_started` carries the level's own name in the milestone's
existing `data` value (`mh::records::milestone_event(name, Value::observed_str(levelName))`,
from `GJGameLevel::m_levelName`, `PlayLayer::init`'s own argument). No schema member was added:
`data` was already a value object. If the name could not be read, `data` says
`{"state":"unavailable","reason":…}` rather than guessing.

What is still open is the gate: `mh::compare` reads a milestone's `name` for ordering and never
looks at `data`, so the observed name is recorded but not yet *compared*. Making it a row would
be a change to comparator semantics, which this task was told not to make — so the decision is
open, and until it is made `level.sha256` **stays zero** in
`harness/scenarios/safe-mode-smoke.json`. Filling it now would make a row pass that proves
nothing.

Still missing before an M3 run:

- **Save isolation.** The fixture cannot be installed into a live save (see above), so either
  `%LOCALAPPDATA%` redirects GD's save directory or the run happens in a VM.
- **Proof GD loads the generated file.** Only a run closes that. Until then the manifest keeps
  its placeholder and the gate keeps saying `blocked` on that row — which is the honest state.
