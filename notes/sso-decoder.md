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

## Correction (superseded): `FUN_18007CB60` and the FNV constant

This section previously read "Correction: `FUN_18007CB60` is an FNV-1a hash, not a keyed
lookup", asserting the function only hashes and does not look anything up. **That claim was
wrong and has been replaced.** It came from reading the hashing half of the function and
missing the lookup that follows it — the same mistake made once already in
[settings-registry.md](settings-registry.md), which records the full history.

The settled position, from `pe_disasm.py` on the whole function and from `keyFor`'s use of a
neighbouring function:

| Function | VA | Direction |
| -------- | -- | --------- |
| `FUN_18007CB60` | `0x18007CB60` | **name → `Tag`**, the body of exported `tagFor` |
| `FUN_18007CCF0` | `0x18007CCF0` | **`Tag` → name**, the body of exported `keyFor` |

Both hash with FNV-1a-64 — that part was always right — and both then probe a chained map.
The original claim also asserted `param_3[2]`/`param_3[3]` are the string's size and
capacity; those displacements are a `std::string` argument, and they belong to the hashed
key, not to the function's return.

One consequence here survives unchanged and is still useful:

> **Mega Hack hashes strings with FNV-1a-64** — the same primitive the persistence sidecar
> uses for `FNV1aChecksumOfRecords` in `RE_NOTES_PERSISTENT_BEST_RUN.md`. One hashing
> convention spans the mod core and the patch, which is useful when writing validators.

The other consequence does not: the four `FUN_18007CB60` sites in the settings table are
not "hashes of setting names". They are profile-key lookups made by `FUN_18008ABC0`, and
their names come from a profile JSON at runtime.

### A caution on the URL recorded above

The endpoint in the preceding section is recorded as UNVERIFIED and untested, which is
correct handling. Add to that: its *provenance* is also unestablished — nothing in the
binary or the sample set was cited as its source, so it should not be treated as a known
Mega Hack service, and it should not be reproduced elsewhere in this repository without a
citation.

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

### A fourth mode the decoder could not see: the whole store was skipped

Found while trying to decode the serialiser group keys in `FUN_180076EB0`
(`notes/settings-registry.md`). `decode_sso.py --file` on that function returned
**0 literals**, even though the function contains `V_BOOL`, `V_INT`, `V_STRING` and
`V_BYTES` as immediates.

Cause: `STORE_RE` demanded a bare hex literal after `=`. Where the compiler typed the local
as a pointer, Ghidra emits the store as

```
local_c8 = (longlong *)0x4c4f4f425f56;      // "V_BOOL"
local_c8 = (longlong *)0x544e495f56;        // "V_INT"
```

and the cast made every one of them fail to match. Nothing was rejected by the plausibility
gate; the evidence was never read.

Fixed by making the cast optional in the pattern. This is a **syntactic** variant of the
same evidence, so it deliberately leaves `plausible()` alone.

| Tree | Decoded | Distinct |
| ---- | ------: | ------: |
| Before | 677 | 495 |
| After | 736 | **525** |

68 literals gained, 9 lost. Every one of the 9 is an improvement or a duplicate: `CTORZ`
(×3) and `LCONFIG` were merge artefacts, `COREINTERFACE` (×2) now reads `COREINTERFACE*`,
`/SET` now reads `KEYBINDS/SET`, and `scroll` was a truncation of `content-scroll`, which
now decodes correctly alongside `tab-scroll` and `tab-backdrop`. `drop` is still recovered,
once instead of three times.

### A rejected variant: emitting a delimiter for zero stores

The same investigation suggested an obvious second fix. A store of pure zeros rstrips to
nothing, so it contributes no delimiter — and in `FUN_180076EB0` the store
`plStack_c0 = 0;` was the only thing separating `V_BYTES` from what followed it. Emitting
one `0x00` for such a store recovered `V_BYTES`, which looked like a clear win.

It was not. Across the tree:

| Tree | Decoded | Distinct |
| ---- | ------: | ------: |
| Cast fix only | 736 | **525** |
| Cast fix + zero delimiter | 771 | 521 |

Distinct count went **down**. The zero stores sit inside literals as well as between them,
so the extra delimiter over-split runs and cost about 48 real literals: `libcocos2d.dll`
(×30), `size`, `label`, `Enter`, `KEYBINDS/REMOVE` (×3), `HOME/RULESETS/0`,
`ICONIC/OVERRIDE`, `fmod.dll`, `HACK_LEVEL_EDIT`. One gain against 48 losses.

**Rejected, and not shipped.** The reasoning it came from — "a zero store is a delimiter, so
emit a delimiter" — is sound in the abstract and wrong in the data, because the same store
plays both roles. Recorded here so the idea is not re-proposed.

`V_BYTES` is instead decoded by hand from `0x53455459425f56` with `pe_disasm.py`, which is
what the verify-twice rule asks for anyway.

### A remaining gap: single-byte tail stores

The same disassembly shows why one of the five keys is invisible to the decoder. MSVC does
not always build a short `std::string` with one wide immediate. For a 9-byte literal in
`FUN_180076EB0` it emits:

```
movabs rax, 0x414d494345445f56   ; "V_DECIMA"
mov      qword ptr [rbp-0x50], rax
mov      byte  ptr [rbp-0x48], 0x4c ; 'L'
```

Ghidra collapses the `mov byte` into `plStack_c0 = (longlong *)0x4c`, which the store
pattern does not match, so the tail character is lost. The same shape appears for
`V_INT` (`mov dword …, 0x4e495f56` + `mov byte …, 0x54`), where the literal never even
appears contiguously in `.text` — it exists only in immediates.

Open work: a rule for single-byte tail stores. It has the same shape of risk as the
rejected zero-delimiter variant above, so it should be measured the same way — count
distinct literals before and after across the whole tree, and list what was lost, not just
what was gained.

## Conclusion

1. SSO literals are mechanically recoverable, and doing so was worth more than expected: it
   corrected a wrong identification of `FUN_18007CB60`, and surfaced the
   `/api/mega_hack/v9/login` endpoint.
2. Mega Hack hashes strings with FNV-1a-64, matching the sidecar's checksum.
3. The decoder has recall, not precision. Confirm before recording.

## The store layout is partly recoverable — offsets are in the local names

The open question above says this "needs the disassembly rather than the decompiler's local
names". That is half right, and the half that is wrong is worth correcting: **Ghidra encodes the
stack offset in the local's own name**. `local_58` is `rbp-0x58`, `uStack_1cc` is `rbp-0x1cc`.
So position is available directly, without disassembly.

| Measured over the recovered tree | Count |
| -------------------------------- | ----: |
| Store assignments | 30,176 |
| … whose local name yields an offset | **16,890 (56.0%)** |
| Files with stores | 5,121 |
| … where *every* store name parses | 1,471 (28.7%) |
| Of those, declared order ≠ address order | **880** |

So for more than half the stores the true byte order is knowable, and in the files that matter it
is knowable completely. `FUN_18003DD90` is the clean example:

```c
local_58   off=-58  = 0x444e494259454b   // "KEYBIND"
uStack_51  off=-51  = 0x53               // "S"
uStack_50  off=-50  = 0x5445532f         // "/SET"   -> one literal, "KEYBINDS/SET"
uStack_4c  off=-4c  = 0x45564f           // "OVE"    -> a different buffer
local_48   off=-48  = 0x11               // size
uStack_40  off=-40  = 0x1f               // capacity = 31 -> buffer boundary
```

### A first attempt at the fix, which made things worse

Grouping stores into runs where consecutive offsets are within 8 bytes — the obvious first
reading — **regresses the exact case it was meant to fix**. In `FUN_18003DD90` the gaps are 7, 1
and 4, all within threshold, so the grouping joined the unrelated `-0x4C` buffer and emitted
`KEYBINDS/SETOVE`, replacing a decode of `KEYBINDS/REMOVE` that was already correct. It also
stopped emitting `KEYBINDS/SET` and `KEYBINDS/REMOVE` at all.

The change was reverted rather than shipped. It is recorded here so the next attempt does not
repeat it, and because it pins down what the real rule has to be.

**Contiguity is the wrong boundary; the buffer is.** An SSO buffer is 16 bytes, and its
size/capacity pair — a small size with capacity 15 or 31, written at `rbp-0x48`/`rbp-0x40` above
— marks where one literal ends and the next begins. Zero stores separate buffers but do not
define them. Grouping by buffer, then ordering stores within a buffer by descending offset, is
the approach that should work; recovering capacity 15 vs 31 to tell a 16-byte buffer from a
32-byte one is the remaining unknown.

## Open questions

- (Resolved 2026-10-20.) FNV-1a-64 is used for **lookup** in both directions: `tagFor`
  (`FUN_18007CB60`) hashes the name to find a `Tag`, and `keyFor` (`FUN_18007CCF0`) hashes
  the `Tag` to find the name, each probing a chained map. See settings-registry.md.
- Decode the SSO immediates for the other 40+ `FUN_18007CB60` callers, which is where most of
  the remaining unknown key names are.
- (Resolved 2026-10-20.) `FUN_1801BA8A0` builds an HTTP request to `/api/mega_hack/v9/login`
  via Geode's web utility (`FUN_1800C08D0`), with a callback (`session::Login::onLogin`).
  This is an online licence/login path and stays a static reading only; it is not exercised.
- Group SSO stores **by buffer** using the size/capacity pair rather than by offset
  contiguity, then order within a buffer by descending offset. The data needed is present and
  measured above; the rejected contiguity approach is documented so it is not retried.
