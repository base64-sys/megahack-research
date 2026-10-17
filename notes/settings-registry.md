# The settings registry

Several notes carried settings as bare numbers. This turns those into one table.

- **Question** — what is Mega Hack's settings registry, how big is it, and which ids matter?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/settings_index.py` → `recovered/functions/_settings.tsv`.

## Accessors

Settings are read through three numeric accessors, all `(this, id)` shaped:

| Accessor | Meaning | Evidence |
| -------- | ------- | -------- |
| `FUN_18008F420` | `getConfigVar<bool>` | returns `bool`; id 245 gates the New Best label |
| `FUN_18002DE50` | `getFlag` | returns `bool`; ids 761/762 gate `cps` fields |
| `FUN_180003D80` | `getConfigVar<scalar>` | result lands in `xmm0`; id 501 is a scale factor |

A fourth call form also exists: `FUN_18007CB60(manager, out, key)`. This is a **keyed lookup in
an FNV-1a-64 chained hash map**, and it has been mislabelled twice here. Recorded once, settled
by disassembly.

The history matters, because both earlier descriptions were wrong in opposite directions:

1. First written as a keyed setting lookup. Right about the behaviour, but it said the key
   "only appears as a Ghidra local", implying the name was unrecoverable.
2. Then "corrected" to *not* a lookup at all, on the grounds that the function contains
   `0xCBF29CE484222325`. That reasoning only looked at the hashing half and missed the lookup
   that follows it.
3. Settled: it hashes, **and** it looks up.

## What the disassembly actually shows

`pe_disasm.py` at `0x18007CB60`, file offset `0x7BF60`:

| Address | Instruction | Meaning |
| ------- | ----------- | ------- |
| `0x18007CB8B` | `movabs rdx, 0x100000001b3` | FNV-1a-64 prime |
| `0x18007CBC0` | `movabs r9, 0xcbf29ce484222325` | FNV-1a-64 basis |
| `0x18007CBD0`-`0x18007CC03` | 4x `movzx`/`xor`/`imul` | hash loop, **four** multiplies per 4 bytes |
| `0x18007CC37` | `and r10, qword ptr [rcx + 0x88]` | `hash & mask` — bucket index |
| `0x18007CC46` | `shl r10, 4` | bucket stride 16 bytes |
| `0x18007CC4A` | `mov r15, [rax + r10 + 8]` | `bucket[i].head` |
| `0x18007CC60` | `cmp rbx, [r15 + 0x20]` | node string length compare |
| `0x18007CC66` | `cmp qword ptr [r15 + 0x28], 0x10` | SSO capacity test |
| `0x18007CC8A` | `call 0x180533B00` | `memcmp` on the key |
| `0x18007CC98` | `mov r15, [r15 + 8]` | `next` — chain walk |
| `0x18007CCC9` | `mov [rsi], rax` | store value to `*param_2` |

So the manager object holds a hash map:

| Offset | Field |
| ------ | ----- |
| `+0x60` | sentinel / end-of-chain node |
| `+0x70` | pointer to the bucket array |
| `+0x88` | bucket count minus one, used as a mask |

and each node holds a `std::string` key inline (`+0x10` buffer, `+0x20` size, `+0x28`
capacity) with the value at `+0x30`. Chain order is insertion order, walked from
`bucket[i].head` at `+0x08`.

This is why `notes/fnv-hash-probe.md` finds no precomputed table: the map is **built during
initialisation**, not stored in the image. That negative result and this structure agree.

## Consequence for key names

The key names are recoverable, and two routes exist:

- `scripts/decode_sso.py` rebuilds them from the immediates at the 40 call sites.
- Insertion sites reveal them directly, since a chained map keeps the key in the node.

What is *not* recoverable from the call shape alone is the meaning of `param_1`. It is used
only as the map owner (`rcx`), never dereferenced for a name in this function.

This also matters beyond labelling: **Mega Hack hashes strings with FNV-1a-64**, which is the
same primitive the persistence sidecar uses for `FNV1aChecksumOfRecords` in
`RE_NOTES_PERSISTENT_BEST_RUN.md`. One hashing convention spans the mod core and the patch.

`FUN_18008ABC0` and `FUN_18008DF30` both decode to the literal `CONFIG`, so the
configuration namespace is a real, named concept in the registry.

## Evidence — the sweep

```
recovered functions    : 18825
distinct numeric ids   : 134
total id reads         : 177
id range               : 4..881
```

Most-read ids:

| Id | Reads | Accessor |
| -- | ----: | -------- |
| 445 | 9 | `getConfigVar<bool>` |
| 447 | 5 | `getFlag` |
| 446 | 4 | `getFlag` |
| 502 | 4 | `getConfigVar<bool>` |
| 869 | 4 | `getFlag` |
| 487, 500, 501, 868 | 3 each | mixed |

Ids are not contiguous. There are gaps — for instance nothing reads 823 through 868 except
868 itself, and the whole 503–722 band is empty. That is consistent with a registry whose
entries were reserved and abandoned, or with several independent id spaces sharing one
numbering scheme.

UNVERIFIED: whether the range really is ~880 settings. Only 134 are read through these three
accessors; others may be reached by a fourth accessor not yet identified, or by the key-based
form.

## The seven previously-open ids, all confirmed

Notes across this repo listed settings 245, 247, 501, 502, 761, 762 and 801 as unidentified.
All seven are present and located:

| Id | Reads | Reading function | Role |
| -- | ----: | ---------------- | ---- |
| 245 | 2 | `FUN_180169EB0`, `FUN_180169F20` | suppress the New Best label |
| 247 | 1 | — | gate on the label scale path |
| 501 | 3 | `getConfigVar<scalar>` | label opacity scale |
| 502 | 4 | `getConfigVar<bool>` | master switch for the scale path |
| 761 | 1 | `FUN_18024E8D0` | include a term in the `cps` display |
| 762 | 1 | `FUN_18024E8D0` | include a second term |
| 801 | 1 | `FUN_18024E8D0` | include the `jump` token |

501 and 502 sit together: 502 enables the behaviour and 501 supplies its value. That is a
repeated shape in this registry — ids `445`/`446`/`447`, `487`/`500`/`501`/`502`,
`868`/`869` are all read together in one function.

## Completeness: the accessor list matters

The first sweep recognised only two accessors and found **107** ids. Setting **501 was
absent** from that run, even though it had already been recorded by hand in
[new-best-label.md](new-best-label.md). Adding `FUN_180003D80` took the count to 134 and
501 appeared with 3 reads.

An index built from a guessed list of accessors is not a registry, it is a sample of one.
The accessors must be enumerated from the binary, not assumed.

## Known limitation: key-based lookups are not recovered

The four `FUN_18007CB60` call sites are indexed, but their key arguments appear as
`local_28`, `local_30`, `local_58` and `local_80` — Ghidra locals, not names.

This is the same SSO problem as [definition-keys.md](definition-keys.md#evidence--how-a-row-is-built):
Mega Hack builds key strings on the stack from immediates, so the name only exists as
`0x4E4F4349`-style constants that have to be decoded by hand. `FUN_18011FFE0` yielded
`ICONIC/DISABLE`, `ICONIC/GENERAL` and friends that way.

So `scripts/settings_index.py` deliberately reports the locals rather than guessing.
`scripts/decode_sso.py` recovers most of them; see
[finding-code.md](finding-code.md).

## Correction: FUN_18007CB60 is an FNV-1a hash

Originally described here as the key-based lookup form. That was wrong, and the
`decode_sso.py` pass is what exposed it — the apparent `'%#"'` format-string fragment was
actually part of the FNV-1a constant. `FUN_18007CB60` takes `(owner, out, key)`, reads the
key's size and capacity from the `std::string`, and returns FNV-1a-64. So those four sites
are hashes of SSO-built strings, not registry lookups, and the setting names they appeared
to hold are not recoverable from the call shape at all.

UNVERIFIED: what the first parameter is, and whether the hash result is used as a key
lookup, a comparison, or only for change detection.

## Conclusion

1. The registry has at least 134 read numeric ids spanning 4–881, through three accessors,
   plus a key-based form.
2. All seven ids previously listed as unknown are located and their roles partly identified.
3. Ids 501/502 and 445/446/447 are read as groups, which is where a session/global switch
   would most plausibly sit.
4. Any index built from an assumed accessor list undercounts. Enumerate, do not assume.

## Open questions

- Decode the SSO immediates at the four `FUN_18007CB60` sites to recover their key names.
- Find a fourth accessor, if one exists, and re-run to get a true count.
- Identify the registry's own name table. Ids are numeric in code, so names must live in a
  data structure or the language file; neither has been located.
- Confirm whether the ICONIC row ids (`445`, `446`, `447`, seen as `0x1BD`, `0x1BE`, `0x1BF`
  in `FUN_18011FFE0`) share a namespace with settings ids or are a separate sequence that
  happens to overlap. The overlap is suspicious and unverified.

## Failure modes recorded

- A guessed accessor list produced a plausible table that silently omitted setting 501. The
  omission was only visible because a hand-recorded id from an earlier note failed to appear.
  Cross-check index output against ids already known from other notes.
- Indexing `&local_XX` as if it were a key name would have produced four nonsense entries. It
  is better to report the local than to invent a name.