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

Closing it needs one of:

- redirect the child process's `%LOCALAPPDATA%` (does GD resolve its save dir through the
  environment or through `SHGetKnownFolderPath`? UNVERIFIED — one run with a probe answers it),
- or a VM/profile snapshot as the contract already suggests.

Until one is in place, a fixture must not be installed into the live save directory, and an M3
run is not eligible on the isolation row regardless of how good the comparison is.

## What this unblocks, and what is still missing

Unblocked: a fixture generator can now write a complete `CCLocalLevels.dat` (or a single-level
`.gmd` in the same dialect) from a declarative spec, because both layers — plist and `k4` — are
implemented and tested.

Still missing before the manifest's `level.sha256` can be pinned:

- **Which level-dict keys GD requires** on load. The keys observed in a real level are
  `kCEK`, `k18`, `k36`, `k23`, `k2`, `k4`, …; the minimum set is not established, and a
  guessed minimum that GD tolerates on one build is not a format.
- **A playable object string.** The grammar is visible (`kS38` colour channels, then
  `id,x,y,...;` objects) but a shape that reliably produces `postUpdate`, `resetLevel` and
  button events for a few hundred frames is a design choice, not a fact.
- **Proof GD loads the generated file.** Only a run closes that; the fixture must therefore
  stay out of the manifest's hash until then, or the manifest would claim an observation
  nobody made.

`level.sha256` stays a placeholder for now, and the gate keeps saying `blocked` on that row —
which is the honest state.
