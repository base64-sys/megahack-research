# Mega Hack's FNV-1a-64 usage — and the hash table that is not there

- **Question** — `FUN_18007CB60` (`0x18007CB60`, 40 callers) hashes a `std::string` to a
  `uint64_t`. If those hashes were precomputed into a table, every setting key could be
  recovered by hashing known strings and looking the value up. Are they?
- **Answer** — **No.** There is no precomputed hash table in any file-backed part of the
  image.
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

## Two variants, neither matching

The decompiled loop is 4x-unrolled, and its parenthesisation is genuinely hard to read by eye.
Counting the prime multiplies in the expression suggests **five** per four bytes, which would
not be textbook FNV-1a:

```
uVar6 = ((ulonglong)b[i+3] ^
         ((ulonglong)b[i+2] ^
         ((ulonglong)b[i+1] ^ (b[i] ^ uVar6) * P) * P) * P) * P;
```

So the script carries both a `std` variant and an `mh` variant implementing the extra
multiply, and tests both. Neither produces any hit.

UNVERIFIED: which variant `FUN_18007CB60` actually implements, and hence the exact hash value
for any given string. The table search cannot discriminate between them, because neither
appears in the image. Settling it needs the disassembly at `0x18007CB60`, read instruction
by instruction, or a runtime observation — and both are weaker evidence than this note should
be read as.

The *algorithm family* is solid, though: the basis and prime are explicit in the
decompilation, the empty-string case returns the bare basis, and the size/capacity reads at
`param_3[2]`/`param_3[3]` match a `std::string` being hashed by content rather than by
pointer.

## What the absence implies

No precomputed table means the hash is computed and compared at run time — change detection
or a lookup against a table that is *built* during initialisation rather than stored. That
rules out a whole class of approach: there is no shortcut where a guessed key can be confirmed
by finding its hash already sitting in the binary.

The practical consequence for the reconstruction is that key names have to be recovered from
code, not from data. `scripts/decode_sso.py` and disassembly remain the route, and
`FUN_18007CB60`'s 40 callers are where the remaining unknown names live.

## Conclusion

1. Mega Hack hashes strings with FNV-1a-64 and stores **no** precomputed hash table in
   file-backed bytes.
2. Verified by a positive control, so the negative is trustworthy.
3. The exact inner loop is still unconfirmed, and the two candidate variants are
   indistinguishable from the image alone.

## Open questions

- Read `0x18007CB60` in the disassembler to settle the five-multiply question.
- Determine whether a table is built at runtime, and if so where it lives — the 40 callers of
  `FUN_18007CB60` are the place to start.
- Search `.data`'s zero-fill tail for the same values; the file-backed scan cannot reach
  bytes that only exist once the image is loaded.