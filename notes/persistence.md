# Persistence: what Mega Hack v9.1.3 actually writes

- **Question** — which on-disk stores does the release binary use, and which of the
  persistence artefacts in `samples/` belong to it?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-18.
- **Tools** — a raw byte scan of all four Windows images, `_xrefs.tsv`,
  `pe_disasm.py`, `scripts/pe_rva.py`.

## The correction this note exists for

`samples/absolllute.hackmega/RE_NOTES_PERSISTENT_BEST_RUN.md` documents a `best_runs.bin`
sidecar: a 32-byte header (`char[8] magic = "MHBRDB1\0"`, version, header size, record size,
record count, FNV-1a-64 checksum) followed by 24-byte records of
`uint64 levelKey; double startPercent; double bestEndPercent;`.

**That format is not Mega Hack's.** It is the format invented by the local persistence patch
in this repo's `samples/` workspace. The evidence is a byte scan of all four Windows images:

| Byte string | `_original` | `_pre_persistence` | `_working_aslr` | generated `.dll` |
| ----------- | ----------: | ----------------: | -------------: | ---------------: |
| `best_runs` | — | — | 1 | 1 |
| `best_runs.bin` | — | — | 1 | 1 |
| `MHBRDB1` | — | — | — | 2 |
| `Octavus` | 1 | 1 | 1 | 1 |
| `hackpro.json` | 1 | 1 | 1 | 1 |
| `data.json` | 1 | 1 | 1 | 1 |
| `save.json` | 1 | 1 | 1 | 1 |

Two independent angles agree: the byte scan, and `_xrefs.tsv`, in which `best_runs` and
`MHBRDB1` return **zero** referencing functions while `Octavus` returns one.

So the sidecar appears only in binaries produced by `build_persistence_patch.py`. Quoting it
as Mega Hack's format would be exactly the error `AGENTS.md` names: quoting an experiment as
evidence about the release. `docs/ROADMAP.md` did so, and this note corrects it rather than
adding a second version of the truth.

This does **not** make the sample note worthless — its geometry, hook map and the two
recorded crash modes (the mutable-`.text` build failing at `DLL+0x28638E`, the
absolute-preferred-base `.data` build failing under ASLR) are about *patching* Mega Hack and
remain accurate as such. It simply is not evidence about what v9.1.3 does.

## What the release does use

Three stores, all confirmed present in `_original.dll`.

### 1. `hackpro.json` — the mod's own configuration

In the mod directory. Read and written by a matched pair, each 302 bytes:

| Direction | VA | RVA | Shape |
| --------- | -- | --- | -------- |
| read | `0x1800BF190` | `0xBF190` | `std::ifstream`, refcount object `0x120` bytes, open mode `1` |
| write | `0x1800BF400` | `0xBF400` | `std::ofstream`, refcount object `0x118` bytes, open mode `2` |

Both build the path with `FUN_1800BEA70`/`FUN_1800BEAA0` and append via `FUN_1800BF320`.
Both have **zero direct callers**, so `_callers.tsv` has no row for them — a concrete case
for the rule that a missing caller row means zero callers, not dead code.

Contents are parsed by `FUN_18008B780`; see
[settings-registry.md](settings-registry.md#the-insert-and-where-names-come-from). In short,
each JSON member's key is a setting name and its `"tag"` number is a 64-bit `hackpro::Tag`,
with optional `"default"`, `"action"` and `"nosave"`.

### 2. GD's `data.json` and `save.json` — the game save files

`FUN_180042FB0`, VA `0x180042FB0`, RVA `0x42FB0`, 1,587 bytes, one caller. It resolves a base
directory, appends the literal `Octavus` (VA `0x18053FBA5`, the only reference), then reads
`data.json` into its second argument and `save.json` into its third, both with the same
open mode the `hackpro.json` reader uses.

The base-directory resolver is `FUN_1800C5210`, VA `0x1800C5210`, 98 bytes:

```
iVar1 = SHGetKnownFolderPath(&DAT_1805820e0, 0, 0, &local_10);
```

The 16 bytes at VA `0x1805820E0` (RVA `0x5820E0`) read back as
`f1b32785-6fba-4fcf-9d55-7b8e7f157091`, which is **`FOLDERID_LocalAppData`**
(`%LOCALAPPDATA%`). So the chain is `%LOCALAPPDATA%\…\Octavus\{data.json,save.json}`.

Failure path: an empty base directory logs the language key `ERROR/HOME/NO_APPDATA`
(VA `0x18053FB8F`).

UNVERIFIED: the directory segment between `%LOCALAPPDATA%` and `Octavus`.
`_xrefs.tsv` contains **no** reference to the literal `GeometryDash` anywhere in the binary,
so that segment is not a hard-coded string here; it is either supplied by Geode's
directory abstraction or derived at runtime. Confirming it needs the Geode host contract for
save-data paths — see [references/](references/).

### 3. Nothing else found

`pe_xrefs.py` over the string inventory turned up no other persistence filename in the
release. Absence of a filename is not proof of absence of persistence, so this is a bounded
result: **the three above are every store this pass found**, not an exhaustive proof.

## Open questions

- **Where does v9.1.3 natively persist Best Run data?** The strongest candidate is GD's own
  `data.json`, since it already carries per-level progress and the Best Run feature is
  `(level, startPercent) -> bestEndPercent`, which maps onto GD's level data. Not yet
  traced; this is the real M4 question now that the sidecar framing is gone.
- The `%LOCALAPPDATA%` → `Octavus` path segment.
- Whether `hackpro.json` write-back happens on a timer, at shutdown, or both.
- `FUN_180042FB0` has one caller, `FUN_180042D70`; whether it is a startup-only read or also
  a migration path is untraced.

## Failure modes recorded

- **Promoting an experiment to a specification.** A long, detailed, internally consistent
  document about `best_runs.bin` reads like ground truth, and it is easy to accept it as
  "Mega Hack's format" because it is the only format anyone has written down in detail.
  The check that settles it is one byte scan of the ground-truth image. Run that before
  believing any format claim, including your own.
- `pe_xrefs.py <image> best_runs` did not return within 120 s and the shell terminated the
  command. A substring that does not occur in the binary appears to be the worst case for
  its search. Prefer `_xrefs.tsv` for existence questions about known strings, and a direct
  byte scan when the question is "is this string present at all".