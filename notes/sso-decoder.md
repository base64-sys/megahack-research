# Recovering SSO string literals, and what they reveal

- **Question** — `settings_index.py` reported four "key-based lookups" as Ghidra locals.
  What are those strings, and what does the rest of the tree hide the same way?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/decode_sso.py`.

## The problem

When Ghidra sees a short `std::string` built on the stack it renders the bytes as a chain of
typed locals, because that is what MSVC emits: an inline 16-byte SSO buffer, then a size and
a capacity of 15. A function that reads

```c
local_58 = 0x4e4f4349;    uStack_54 = 0x4349;
uStack_52 = 0x2f;         uStack_51 = 0x41534944;
uStack_4d = 0x454c42;
```

actually contains `ICONIC/DISABLE`. The text is present but unreadable, which is why
`notes/definition-keys.md` had to decode `FUN_18011FFE0` by hand.

## Method

`scripts/decode_sso.py` concatenates every immediate store in a file, in declaration order
(which is the order MSVC emitted the bytes), and pulls out printable runs. Two details
matter:

- **Per-store padding must be stripped.** Each store is a 4- or 8-byte immediate holding a
  few characters followed by zero padding. Without stripping it, every literal is cut at a
  4-byte boundary — `ICONIC/DISABLE` comes out as `ICONIC` and `DISABLE`.
- **Literal-separating zeros must be kept.** They come from the buffer initialisation and the
  size/capacity fields, which are separate stores, and they are what stop one key running
  into the next.

`builtin_strncpy` calls are read directly, since Ghidra keeps the literal there.

## Evidence — self-check and results

On the known function `FUN_18011FFE0`, which had been decoded by hand:

| Decoded | Matches hand decoding |
| ------- | --------------------- |
| `v9.1.3` | yes — independently matches `mod.json` |
| `ICONIC/DISABLE` | yes |
| `ICONIC/RAND` | yes |
| `ICONIC/GENERAL` | yes |
| `ICONIC/EFFECTS` | yes |
| `HOME/DOT` | yes |
| `ICONIC/VANILLA_PROGRESS_BAR` | yes |

`v9.1.3` is a useful independent check: the decoder recovered the mod's own version string
out of code without ever being told it.

Across the tree it recovers keys that the earlier definition-key inventory had listed only as
opaque `DAT_` addresses, including `MESSAGE/CANCEL`, `MESSAGE/OK`, `ERROR/DISMISS`,
`KEYBINDS/EMPTY`, `KEYBINDS/SET`, `KEYBINDS/REMOVE`, `HOME/SEARCH`, `HOME/SEARCH/AUTO_SELECT`,
`HOME/HOTKEY_ICONS`, and `DISPLAY/HACK_BORDERLESS_CLASSIC`.

It also confirms `CONFIG` as a literal in both `FUN_18008ABC0` and `FUN_18008DF30`, so the
configuration namespace is a real named concept rather than an inference.

## Merged into the string index

`build_xref_index.py` now calls `decode_from_file` directly, so stack-built literals appear
in `_xrefs.tsv` alongside VA-backed ones rather than living in a separate pass. They get a
separate `sso` marker and no VA, because the bytes only ever exist in a stack buffer:

```
sso  [4 refs]  'KEYBINDS/SET'
      18003d470_FUN_18003d470
      18003d600_FUN_18003d600
      18003dd90_FUN_18003dd90
      18003f870_FUN_18003f870
```

| Index                    | Rows  |
| ----------------------- | ----: |
| VA-backed strings       | 3,611 |
| stack-built literals    |   537 |
| **total distinct**      | **4,148** |

A consequence worth knowing: the index now contains both a whole key and its fragments, e.g.
`KEYBINDS/SET` and `/SET`, because the same run of stores can be split differently by the
zero boundaries. That is the false-merge/split limitation below, now visible in the index
rather than hidden in a separate tool's output.

## A new endpoint, confirmed twice

The merged index surfaced `/api/mega_hack/v9/login`, which no note had recorded. Confirmed
from two independent angles:

| Angle | Evidence |
| ----- | -------- |
| String xref | literal at VA `0x18057e55c`, one reference |
| SSO decode  | same text rebuilt in the immediate stream of the same function |

Both name `FUN_1801BA8A0`, so this is one function that builds the path on the stack and also
refers to the stored copy. A sibling `/api/mega_hack/v8/logout_all` appears in the same
pass. UNVERIFIED: the transport, the payload, and whether `v8` here is a fallback for older
clients rather than a distinct service. Do not test against it.

## Correction: `FUN_18007CB60` is an FNV-1a hash, not a keyed lookup

`notes/settings-registry.md` described `FUN_18007CB60(manager, out, key)` as a key-based
settings lookup, with the key appearing only as a Ghidra local. **That was wrong.**

Decompiling it shows `0xCBF29CE484222325`, the FNV-1a-64 offset basis, and it reads
`param_3[2]` as the string size and `param_3[3]` as the capacity, spilling to the heap above
15. So the signature is:

```
uint64_t FUN_18007cb60(const char *owner, uint64_t *out, const std::string &key)
```

— **an FNV-1a-64 hash of a string.** The apparent `'%#"'` string the decoder surfaced was a
fragment of the hash constant, which is exactly the kind of coincidence that makes an index
look broken when it is not.

Two consequences:

1. The four "key-based lookups" in the settings table are hashes, not lookups. The setting
   names are not recoverable from the call shape at all; the correct way to read them is the
   SSO decoder, or the disassembly of the stores.
2. **Mega Hack hashes strings with FNV-1a-64** — the same primitive the persistence sidecar
   uses for `FNV1aChecksumOfRecords` in `RE_NOTES_PERSISTENT_BEST_RUN.md`. One hashing
   convention spans the mod core and the patch, which is useful when writing validators.

UNVERIFIED: what the first parameter is, and whether the hash feeds a lookup, a comparison,
or only change detection.

## Known limits of the decoder

Three error modes, all visible in real output, none papered over:

- **False merges.** Two adjacent literals with no separating zero run together:
  `FUN_1800238A0` and `FUN_180026F90` both yield `COREINTERFACE`, which is `CORE` and
  `INTERFACE` sitting next to each other. `CORE` is independently confirmed by the
  readiness probe in `notes/new-best-label.md`.
- **Splits.** The mirror image of the above: `KEYBINDS/SET` also appears as `/SET` when a
  different zero boundary cuts the same run.
- **Truncation.** A literal can lose its leading character when the first store is wider
  than the text: `HOME/HOTKEY_ALT` and `HOME/ANIM_SPEED` decode as `OME/HOTKEY_ALT` and
  `OME/ANIM_SPEED`.

`plausible()` gates out the obvious non-strings — offsets and numbers whose bytes happen to
be printable, like ` GD`, `(nL`, `/3E` — by trimming the run and requiring at least two
alphabetic characters. That took the distinct count from 715 to 537. The gate is a
recall-first filter, not proof, and it is deliberately not tuned against any individual
sample: the goal is to reject a class of noise, not to memorise the answers.

So treat the output as a high-recall candidate list to be confirmed, never as a finished
inventory. Every string here should be re-derived from the disassembly before it goes into a
note as fact — which is what the AGENTS.md verify-twice rule already requires.

## Conclusion

1. SSO literals are mechanically recoverable, and doing so was worth more than expected: it
   corrected a wrong identification of `FUN_18007CB60`, and surfaced the
   `/api/mega_hack/v9/login` endpoint.
2. Mega Hack hashes strings with FNV-1a-64, matching the sidecar's checksum.
3. The decoder has recall, not precision. Confirm before recording.

## Open questions

- Determine whether the FNV-1a hash is used for lookup, comparison or change detection.
- Decode the SSO immediates for the other 40+ `FUN_18007CB60` callers, which is where most of
  the remaining unknown key names are.
- Establish what `FUN_1801BA8A0` does with the login path — offline by assumption, so this
  stays a static reading and is not to be exercised against a live service.
- Recover the store layout precisely enough to fix the split/merge error modes, which needs
  the disassembly rather than the decompiler's local names.
