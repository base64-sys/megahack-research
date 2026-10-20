# Mega Hack's FNV-1a-64 usage — and the hash table that is not there

- **Question** — `FUN_18007CB60` (`0x18007CB60`, 40 callers) hashes a `std::string` to a
  `uint64_t`. If those hashes were precomputed into a table, every setting key could be
  recovered by hashing known strings and looking the value up. Are they?
- **Answer** — **No.** There is no precomputed hash table in any file-backed part of the
  image. The disassembly explains why: the map is built at run time, and the hash is what
  indexes it.
- **Tool** — `scripts/fnv_hash_probe.py`.
- **Date** — 2026-10-17.

## Why the question was worth asking

A hash table would be a very strong tool. Settings keys are the single largest source of
unresolved names in this reconstruction, and a table keyed by hash would let any recovered
or guessed key be confirmed in one step, without needing to find the code that uses it.

## Method

`scripts/fnv_hash_probe.py` collects every string available from the recovered tree — inline
literals Ghidra decoded, and SSO literals rebuilt from immediates — hashes each with FNV-1a-64,
and searches the image for the 8-byte little-endian result.

| Step | Value |
| ---- | ----- |
| Candidates | 8,709 (3,700 distinct) |
| Hash function | FNV-1a-64, basis `0xCBF29CE484222325`, prime `0x100000001B3` |
| Bytes scanned | all file-backed bytes of `absolllute.hackmega_original.dll` |
| Hits | **0**, for both inner-loop variants |

## The control, which is the part that matters

A "zero hits" result is worthless unless the scan is known to be able to find something. The
script therefore runs a positive control first, and refuses to report results if it fails.

The FNV basis `0xCBF29CE484222325` is loaded by `FUN_18007CB60` as a `movabs` immediate, so
its little-endian bytes must exist in `.text`. The control finds them:

```
control OK: FNV basis found at 0x180003dcd (file offset 0x31cd)
```

So the scanner works, and the zero result is a real absence rather than a bug.

This mattered in practice: the first implementation searched the image once per candidate
value, which is O(3,700 × 8.7 MB). It had reported "0 hits" for the four hand-supplied test
strings, which was correct — but the full sweep under that implementation would never have
finished, so the full sweep was only attempted after rewriting `scan` to slide the window
across the image once.

## Two variants — the `mh` variant was wrong, and the disassembly says why

The decompiled loop is 4x-unrolled, and counting the prime multiplies in the decompiled
*expression* suggests five per four bytes, which would not be textbook FNV-1a:

```
uVar6 = ((ulonglong)b[i+3] ^
         ((ulonglong)b[i+2] ^
         ((ulonglong)b[i+1] ^ (b[i] ^ uVar6) * P) * P) * P) * P;
```

So the first version of this tool carried both a `std` variant and an `mh` variant
implementing the extra multiply, and tested both. Neither hit.

`pe_disasm.py` at `0x18007CB60` settles it. The loop body is four `imul r11, rdx`:

| Address | Instruction |
| ------- | ----------- |
| `0x18007CBD8` | `imul r11, rdx` |
| `0x18007CBE5` | `imul r9, rdx` |
| `0x18007CBF2` | `imul r11, rdx` |
| `0x18007CBFF` | `imul r9, rdx` |

Four multiplies for four bytes. **It is textbook FNV-1a-64**, and the `mh` variant was an
artefact of reading the decompiler's parenthesisation by eye. The `mh` variant has been
removed from the tool rather than left in place as an option that never matches anything.

Kept here because it is the same failure as the two wrong descriptions of `FUN_18007CB60` in
`notes/settings-registry.md`: trusting a shape read off a decompiled expression instead of
counting the actual instructions. `pe_disasm.py` is the ground truth; the decompiler is a
reading of it.

## The hash *is* used for lookup

An earlier version of this note concluded that the hash was "computed and compared at run
time for change detection, not used for lookup." **That was wrong**, and the disassembly
shows it: immediately after the hash loop,

```
0x18007cc37  and   r10, qword ptr [rcx + 0x88]   ; hash & mask -> bucket index
0x18007cc46  shl   r10, 4                        ; 16-byte bucket stride
0x18007cc8a  call  0x180533b00                   ; memcmp against the stored key
0x18007cc98  mov   r15, qword ptr [r15 + 8]      ; next -> chain walk
0x18007ccc9  mov   qword ptr [rsi], rax          ; write the value out
```

So `FUN_18007CB60` is a keyed lookup in a chained hash map, hashed with FNV-1a-64. The
manager at `rcx` holds a sentinel at `+0x60`, a bucket array at `+0x70`, and the bucket mask
at `+0x88`. Full field table in [settings-registry.md](settings-registry.md).

This does not change the negative result — there is still no precomputed table in the image.
It explains it: **the map is built during initialisation**, so its contents cannot appear as
stored bytes. The two findings agree, and neither contradicted the other. What was wrong was
the inference about *why* the hashes were absent.

## What the absence implies

No precomputed table means a guessed key cannot be confirmed by finding its hash already
sitting in the binary. The practical consequence for the reconstruction is that key names have
to be recovered from code: `scripts/decode_sso.py` at the 40 call sites, or the map's
insertion sites, which keep the key inline in each node.

## Conclusion

1. Mega Hack hashes strings with FNV-1a-64 — confirmed at instruction level, four multiplies
   per four bytes.
2. That hash drives a chained hash-map lookup, not merely a comparison.
3. The map is built at run time; no precomputed table exists in file-backed bytes.
4. The negative is trustworthy: verified with a positive control.

## Open questions

- (Resolved 2026-10-20.) The map's constructor/insertion sites are documented in
  settings-registry.md: `FUN_1800A27E0` builds the registry object at `0x180800F58`, and
  `FUN_18008B780` parses `hackpro.json` and interns each key via `FUN_18007C700`, so the
  keys are data, not binary literals. UNVERIFIED: the initial bucket mask at `+0x88` and
  whether the zero-fill tail holds a pre-built map.
- (Unchanged 2026-10-20.) `[rcx + 0x88]` is the bucket mask for the chained map, but its
  value is established at runtime by `FUN_1800A27E0`/`FUN_1800A28C0` and is not present in
  file-backed bytes, so the initial bucket count is not statically recoverable. The zero-fill
  tail likewise has no file-backed image. UNVERIFIED: both need a live load.