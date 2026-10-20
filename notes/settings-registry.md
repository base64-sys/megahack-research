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

## Map accessors found so far

Searching for functions that touch all three manager fields (`+0x60`, `+0x70`, `+0x88`)
returns 21 candidates. Most are false positives — unrelated structs that happen to have an
8-byte field at the same offset. Three are confirmed members of this map, all of which
reproduce the same bucket arithmetic `(hash & mask) * 0x10` and chain walk via `+0x08`:

| Address | Size | Role | Callers |
| ------- | ---: | ---- | ------: |
| `0x18007CB60` | 365 | **`tagFor`: name → `Tag`**, the universal name resolver | 40 |
| `0x18007C700` | 957 | **`internTag`: insert a `Tag`↔name pair into both maps** | 5 |
| `0x1800788D0` | 11,901 | full-map iterator, `callers: 1` | 1 |

`FUN_1800788D0` is large enough to be walking every entry, which is the kind of function that
serialises or compares the whole registry.

**The insert has since been found**, and so has the answer to "where are the names". Both
were settled from the call graph rather than by pattern-grepping; the section
[The insert, and where names come from](#the-insert-and-where-names-come-from) below
supersedes this one. What was tried first, and why it failed:

- grep for a write of `0x10` into a `+0x28` capacity slot — one hit, `FUN_1804EA750`, which
  is `FindFirstFile`/`WIN32_FIND_DATAW` file enumeration. The pattern was too loose; the
  offset is not distinctive enough on its own.
- The two co-located candidates beside the lookup (`0x1800788D0`, `0x18007C700`) both turned
  out to be readers.

Both failures share a cause worth recording: the search looked for **writes** to the map,
but the insert's distinguishing feature is that it writes to **two** maps and hashes a
`std::string` on the way (`0x100000001b3` appears in a 4-bytes-per-iteration loop plus a
tail, not a straight-line 8-byte unroll). Grepping for the hash constant finds 163
functions, which is too many to be useful; reading the callers of the already-known lookup
found it in one step.

## Consequence for key names

The key names are recoverable, and two routes exist:

- `scripts/decode_sso.py` rebuilds them from the immediates at the 40 call sites.
- Insertion sites reveal them directly, since a chained map keeps the key in the node.

The second route is the real one, and it terminates in a file rather than in the binary: see
[The insert, and where names come from](#the-insert-and-where-names-come-from). What is
*not* recoverable from the call shape alone is the meaning of `param_1`. It is used
only as the map owner (`rcx`), never dereferenced for a name in this function.

This also matters beyond labelling: **Mega Hack hashes strings with FNV-1a-64**, which is the
same primitive the persistence sidecar uses for `FNV1aChecksumOfRecords` in
`RE_NOTES_PERSISTENT_BEST_RUN.md`. One hashing convention spans the mod core and the patch.

`FUN_18008ABC0` and `FUN_18008DF30` both decode to the literal `CONFIG`, so the
configuration namespace is a real, named concept in the registry.

## Evidence — the sweep

```
recovered functions    : 18825
distinct tags          : 149
total tag reads        : 192
tag range              : 3..881
accessors recognised   : 4  (FUN_18008F420 bool, FUN_18002DE50, FUN_180003D80 scalar,
                            FUN_18002EE40 hackpro::Bytes)
```

The four are not four registries. `FUN_18002EE40` is the typed accessor for
`hackpro::Bytes` — it logs that literal on a miss — so all four read the one registry with
four different value types.

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

### Found again, in a new place: `FUN_18002EE40`

The warning above has now been confirmed a second time, with a concrete casualty.

`FUN_18002EE40(registry, out, uint64_t Tag)`, VA `0x18002EE40`, RVA `0x2EE40`, folds all
eight bytes of its third argument through FNV-1a-64 and probes a map at `registry + 0xC0`
with the mask at `+0xD8`:

```
lVar5 = ((... FNV-1a-64 ...) * 0x100000001b3 & *(ulonglong *)(param_1 + 0xd8)) * 0x10;
lVar4 = *(longlong *)(*(longlong *)(param_1 + 0xc0) + 8 + lVar5);
```

Same shape as `getConfigVar<bool>`, but over a **different map layout** — not the
`BasicConfig` maps at `+0x20`/`+0x30`/`+0x48` or `+0x138`/`+0x148`/`+0x160`. So this is a
second registry, not a fourth accessor on the first.

`scripts/settings_index.py` did not recognise it, and **tag `0x11D` (285) was therefore
missing from `_settings.tsv`** — the index jumped from 284 to 292 across the gap. It is read
from the `PlayLayer::onQuit` hook body; see
[hook-dispatch.md](hook-dispatch.md#a-settings-accessor-this-repos-index-misses).

This is the second time the same lesson has cost a specific setting: the first was tag 501
missing from the original two-accessor sweep.

### Fixed, and the fix changed the shape of the matcher

`scripts/settings_index.py` previously used one regex for every accessor, taking "the first
comma-separated argument that looks numeric". That is a **guess about argument position**,
and it is exactly the kind of guess this file keeps warning against. The three known
accessors take the tag second; `FUN_18002EE40` takes it **third**, behind an
out-parameter. The old pattern happened to match some of those calls by accident and missed
others, with no error anywhere.

The accessor table now records the argument index explicitly, arguments are split on
top-level commas with nesting tracked (a call like
`FUN_18002ee40(param_1,(vector<unsigned_int,…> *)…)` has a comma inside a cast, which a naive
split would mis-count), and the captured argument must be a bare literal.

Result:

| | Before | After |
| --- | ---: | ---: |
| distinct tags | 134 | **149** |
| total reads | 177 | 192 |
| id range | 4..881 | **3..881** |

The 15 recovered tags are `3`, `131`, `132`, `285`, `303`, `304`, `305`, `321`, `403`, and
`851`–`856`. Every one was verified by reading its call site, not by trusting the count.

**`FUN_1801904F0` reads six consecutive tags — `0x353` to `0x358`** — each into its own
local. An earlier draft read that as "six related settings, therefore one feature". **That
was wrong**: they are six `hackpro::Bytes` values forming a string-prefix dispatch table,
documented above. The "sibling tags mean one feature" rule does not survive contact with
this example, and it was stated too confidently.

UNVERIFIED: whether any other tag group in `_settings.tsv` is a dispatch table rather than a
feature. With 149 tags read by 192 sites, that cannot be answered from the table alone.

### Corrected: `FUN_18002EE40` is `getConfigVar<hackpro::Bytes>`, not a second registry

An earlier draft of this section called `FUN_18002EE40` "a second registry's accessor",
because its map sits at a different offset from `getConfigVar<bool>`'s. **That was wrong**,
and the evidence against it is one line of the function itself:

```
local_60 = "hackpro::Bytes";
FUN_1800202a0(&local_64, 3, local_58, &local_80, &local_70, &local_60);
```

The literal is the *requested type's name*, emitted on a miss alongside the tag. So
`FUN_18002EE40` is the typed accessor for **`hackpro::Bytes`** — the fifth of the five value
types, the one `FUN_180076EB0` writes under `V_BYTES` and the exported API exposes as
`std::vector<unsigned int>`. It is the same registry, and this is the sixth type-aware
accessor found rather than a new key space.

Two of the host-method inventory's non-GD entries are therefore **value types, not host
methods**: `hackpro::Bool`, `hackpro::Decimal`, `hackpro::Int`, `hackpro::String` and
`hackpro::Bytes` are named by these accessors through the same runtime resolver that
resolves GD methods. That is consistent with
[host-methods.md](host-methods.md) flagging them, and it explains why they looked like
`Class::method` strings.

### The registry is a global in `.data`'s zero-fill tail

`FUN_18008A510()` is `FUN_1800A2F90(FUN_1800A2720())`, and both halves are now known:

```
FUN_1800a2f90(longlong param_1) { return param_1 + 0xd8; }

FUN_1800a2720() {
  if (tls_guard < DAT_180801400) {
      FUN_1800a27e0(&DAT_180800f58);        // the static constructor
      atexit(FUN_1800a2890);               // and its destructor
      _Init_thread_footer(&DAT_180801400);
      return &DAT_180800f58;
  }
  return &DAT_180800f58;
}
```

So the settings registry is the **global object at VA `0x180800F58`**, not a heap allocation,
and every accessor receives that global `+0xD8`.

`scripts/pe_rva.py` says where it lives, and the answer explains why every map field reads
as zero in the image:

```
0x800f58  zero-fill  no file offset  0x958 past .data SizeOfRawData; mapped, reads as 0
0x801030  zero-fill  no file offset  0xa30 past .data SizeOfRawData; mapped, reads as 0
0x8010e0  zero-fill  no file offset  0xae0 past .data SizeOfRawData; mapped, reads as 0
0x801190  zero-fill  no file offset  0xb90 past .data SizeOfRawData; mapped, reads as 0
```

**The whole registry has no bytes in the file.** It is populated entirely by
`FUN_1800A27E0` at load. That is the same region as the patch's mutable-state block at
`0x18081BFC0`, and `pe_rva.py` gives the two different verdicts they deserve:

| Address | Verdict |
| ------- | ------- |
| `0x180800F58` (the registry) | `zero-fill` — mapped, reads 0 |
| `0x18081BFC0` (patch state block) | `gap` — past `VirtualSize`, unmapped |

### Constructor, and a second angle on the `+0x128` layout

`FUN_1800A27E0` (`0x1800A27E0`, 97 bytes) calls `FUN_1800A5BA0`, then `FUN_1800A28C0`, then
zeroes three qwords at the end. `FUN_1800A28C0` (359 bytes) contains:

```
*param_1              = hackpro::Config::vftable;
param_1[0x25]         = hackpro::Config::vftable;     // 0x25 * 8 = 0x128
```

This is an **independent confirmation** of the layout derived in
[hook-dispatch.md](hook-dispatch.md) from the `add rcx, -0x128` adjustment in
`FUN_1800917B0`. Two unrelated functions — a thunk and a constructor — agree that
`hackpro::Config` sits at `+0x128` of this object and `hackpro::BasicConfig` at 0.

The constructor's other writes give the object's extent and its initialised members:

| Offset | Written as |
| ------ | ---------- |
| `+0x00` | `hackpro::Config::vftable` |
| `+0x08`, `+0x10` | zeroed by `FUN_1800A5BA0` |
| `+0x50`, `+0x90` | map constructors |
| `+0x128` | `hackpro::Config::vftable` |
| `+0x1B0`, `+0x1C0`, `+0x1D8` | zeroed as a consecutive group |
| `+0x248` | `FUN_1803A50B4(0x28)` sentinel node, self-referential with `+0x18 = 0x101` |
| `+0x490`–`+0x4A0` | zeroed |

So the object is at least **`0x4A8` = 1192 bytes**.

### The `+0xB0` conflict resolved: the two readers use different bases

`FUN_180076EB0` is `hackpro::BasicConfig::vtable[0]`, so it arrives by **virtual dispatch**
and its `this` is `object + 0x128`. The accessors receive `object + 0xD8` from
`FUN_18008A510()`. `pe_disasm.py` at `0x18008A530` shows the serialiser is handed the
unmodified `rcx`:

```
000000018008a557  48 89 d6             mov    rsi, rdx
000000018008a55a  48 89 cb             mov    rbx, rcx
000000018008a55d  e8 4e c9 fe ff       call   0x180076eb0
```

`rcx`/`rdx` are not clobbered; the `mov`s are for later use.

### And `+0xB0` in the serialiser is provably a circular list

Also from `pe_disasm.py` at `0x180076EB0`, the first accesses are:

```
0000000180076edb  48 89 cf                    mov    rdi, rcx                    ; rdi = this
0000000180076f29  48 8b b7 b0 00 00 00        mov    rsi, qword ptr [rdi + 0xb0]  ; list head
0000000180076f40  49 89 f5                    mov    r13, rsi
0000000180076f60  4d 8b 6d 00                 mov    r13, qword ptr [r13]         ; node = node->next
0000000180076f64  49 39 f5                    cmp    r13, rsi                    ; until back to the head
0000000180076f67  0f 84 e1 00 00 00           je     0x18007704e
```

`rsi = *(this + 0xB0)`, then `r13 = *r13` walking, terminating when `r13 == rsi`. That is
the **circular intrusive-list sentinel**, unambiguously — `node->next == node` at the head.
No interpretation is needed; the loop shape settles it.

The same function reads `*(this + 0x98)` at `0x180076F71` and walks it as a **red-black tree**
(`cmp byte ptr [rcx+0x19]`, `cmp qword ptr [rcx+0x20]`, `shl edx,4`), which is the tag set —
consistent with `FUN_18007D1C0`'s `contains` walk in the same file.

### Two different fields, and why there was no conflict

| Reader | Base | Field | Object offset | Shape |
| ------ | ---- | ----- | ------------: | ----- |
| `FUN_180076EB0` (serialiser) | `+0x128` | `+0xB0` | **`+0x1D8`** | circular list head |
| `FUN_18002EE40` (`Bytes`) | `+0xD8` | `+0xB0` | **`+0x188`** | pointer to a bucket array |

**Different fields.** What looked like one field claimed twice was the same offset number read
through two bases — the same class of error as the "second registry" claim, one level down.

### Resolved: The `Bytes` map is an MSVC `std::unordered_map` at `+0x170`

A full disassembly read of `FUN_18002EE40` (VA `0x18002EE40`, RVA `0x2EE40`) and constructor
`FUN_1800A28C0` (VA `0x1800A28C0`, RVA `0xA28C0`) settles the container layout completely:

1. `FUN_1800A28C0` constructs the container at `object + 0x170` via `call 0x1800A42A0`.
2. When accessed via `param_1 = object + 0xD8`, the displacements correspond exactly to the standard
   MSVC x64 `std::_Hash` (`std::unordered_map<uint64_t, std::vector<unsigned int>>`) layout:
   - `[rcx + 0xb0]` (`object + 0x188`): loaded into `rdx` — `_List._Myhead` (end sentinel node).
   - `[rcx + 0xb8]` (`object + 0x190`): `_List._Mysize`.
   - `[rcx + 0xc0]` (`object + 0x198`): loaded into `rcx` — `_Vec._Myfirst` (bucket array base pointer).
   - `[rcx + 0xc8]` (`object + 0x1A0`): `_Vec._Mylast`.
   - `[rcx + 0xd0]` (`object + 0x1A8`): `_Vec._Myend`.
   - `[rcx + 0xd8]` (`object + 0x1B0`): masked via `and r8, [rcx + 0xd8]` — `_Mask` (bucket bitmask).
   - `[rcx + 0xe0]` (`object + 0x1B8`): `_Max_idx`.
3. The apparent "gaps" at `+0x190`, `+0x1A0`, `+0x1A8` were the intermediate members of MSVC's
   standard `_Hash` container not referenced in the lookup loop.
4. If found, the node holds type byte `4` (`V_BYTES`) at `[r14 + 0x38]`, vector begin pointer at
   `[r14 + 0x18]`, and vector end pointer at `[r14 + 0x20]`. Element count is `(end - begin) >> 2`
   (`sar rax, 2`, four bytes per `unsigned int`), allocated via `FUN_1803A50B4` and copied with `memmove`.

### A correction to my own reasoning

The "second registry" claim was made because `FUN_18002EE40`'s offsets differ from
`getConfigVar<bool>`'s. The offsets do differ — but that is now explicable without a second
object, because `FUN_18008A510()` returns **a fixed `+0xD8` into a global whose extent and
layout were unknown at the time**. Comparing an offset read through one accessor against an
offset read through another was never a valid comparison without the base. It happened to
give the right answer for the wrong reason, and then the right answer got the wrong
justification.

## The six-tag group is a string-prefix dispatch

`FUN_1801904F0`, VA `0x1801904F0`, RVA `0x1904F0`, 708 bytes, one caller, reads tags
`0x353`, `0x355`, `0x356`, `0x354`, `0x357`, `0x358` in that order and returns a two-field
result `{int code; bool matched;}`.

It is **not** a settings block that reads six related values. It is a cascade: for each
tag, `FUN_18002EE40` yields a `std::string`, and the code compares the **first four bytes**
of that string against `param_2`, returning the first match's single-character code:

| Tag | Code returned | As a character |
| --- | ------------: | --------------- |
| `0x353` | `0x57` | `W` |
| `0x355` | `0x41` | `A` |
| `0x356` | `0x44` | `D` |
| `0x354` | `0x25` | `%` |
| `0x357` | `0x26` | `&` |
| `0x358` | `0x27` | `'` |

Six string-valued settings whose values are compared as 4-byte prefixes against an `int`
argument, resolving to one of six single-character codes. What that vocabulary is — a
command grammar, a key binding table, a text-mode dispatch — is not established, and the
codes are recorded as the integers plus the characters they happen to spell rather than as a
claimed alphabet.

The guard is also notable:

```
lVar3 = FUN_1801acdb0();                                  // 50 callers, the current level
if ((lVar3 == 0) || (*(char *)(lVar3 + 0x39ef) == '\x01')) {
    *(undefined1 *)(param_1 + 1) = 0;
    return param_1;
}
```

**`<level>+0x39EF` is a second one-byte field**, alongside the `PlayLayer+0x3230`
re-entrancy flag from [hook-dispatch.md](hook-dispatch.md). `FUN_1801ACDB0` returning the
current level is now the third confirmed use of it.

The "sibling settings mean one feature" reading of a tag group was therefore wrong here: it
is a dispatch table, not a feature's settings. Worth recording, because the rule was stated
confidently in the previous draft of this file.

UNVERIFIED: what the six `hackpro::Bytes` values contain and what vocabulary the six
characters belong to. The values live in `hackpro.json`, which this repo does not read.

## Known limitation: tag names are not in the image

The four `FUN_18007CB60` call sites are indexed, but their key arguments appear as
`local_28`, `local_30`, `local_58` and `local_80` — Ghidra locals, not names. That is now
explained rather than merely noted: those sites walk a `std::set<std::string>` of keys read
from a profile JSON object, so the text is user data that only exists at runtime.

`scripts/settings_index.py` deliberately reports the locals rather than guessing, and that
is correct. `scripts/decode_sso.py` recovers SSO-built constants elsewhere;
see [finding-code.md](finding-code.md).

## Removed: the superseded "it is only a hash" claim

An earlier draft of this file carried a section headed "Correction: FUN_18007CB60 is an
FNV-1a hash", asserting the function does not look anything up. That claim was based on
reading only the hashing half and has been **deleted**, because the disassembly table at
the top of this note shows the bucket arithmetic and chain walk that follow it.

What the four `FUN_18007CB60` sites actually do was settled later, from `FUN_18008ABC0`
(below): they are keyed lookups over a `std::set<std::string>` of profile keys, and the
`'\x01'` the caller tests is the found/not-found flag. The key text comes from the profile
JSON, not from the settings registry, which is why `settings_index.py` reports a Ghidra
local there and is right to.

## The registry class: `hackpro::BasicConfig` and `hackpro::Config`

- **Date** — 2026-10-18.
- **Method** — `scripts/rtti_index.py` → `_rtti.tsv`, then a raw vtable read with
  `pefile`, cross-checked against `pe_disasm.py`.

`scripts/rtti_index.py` already knew both types but nothing had followed up on them. Their
vtables were read directly:

| Class | TypeDescriptor / COL VA | Vtable slot 0 | slot 1 | slot 2 |
| ----- | ---------------------- | ------------- | ------ | ------ |
| `hackpro::BasicConfig` | `0x18053E8D8` | `0x180076EB0` | `0x1800788D0` | — (2 virtuals) |
| `hackpro::Config` | `0x180544FD8` | `0x18008A530` | `0x18008ABC0` | `0x1800917B0` |

`0x1800788D0` is the "full-map iterator" already listed above, which is consistent: it is
`BasicConfig::vtable[1]`.

### Config derives from BasicConfig at +0x128

`pe_disasm.py` at `0x1800917B0` (RVA `0x917B0`, file `0x90BB0`) — `Config::vtable[2]`:

```
00000001800917b0  48 81 c1 d8 fe ff ff   add    rcx, -0x128
00000001800917b7  48 8b 12              mov    rdx, qword ptr [rdx]
00000001800917ba  e9 61 dc ff ff        jmp    0x18008f420
```

`rcx` entering a virtual is already adjusted to the subobject, so `-0x128` is exactly the
offset of the `hackpro::Config` subobject within the most-derived object — which places
the `hackpro::BasicConfig` base at offset 0 and its vtable at `+0x128`. `0x18008F420`,
`getConfigVar<bool>`, is a **non-virtual** member of the base.

Two independent angles agree: the RTTI walk says `Config` has its own vtable, and the
adjustment `rcx - 0x128` is the only way `Config::vtable[2]` can reach a function that lives
on a different subobject. Sizes of the base are not established; the first
`BasicConfig`-derived field observed is the entry list at `+0xB0`.

## The key is a 64-bit `hackpro::Tag`, not a small number

This is the finding that reframes everything above it.

`pe_disasm.py` at `0x18008F420`, file `0x8E820`, hashes **all eight bytes** of its second
argument:

```
000000018008f45c  41 89 db              mov    r11d, ebx
000000018008f45f  41 c1 eb 10           shr    r11d, 0x10
...
000000018008f486  0f b6 d3              movzx  edx, bl
000000018008f489  48 bf 25 23 22 84 e4 9c f2 cb   movabs rdi, 0xcbf29ce484222325
```

eight `xor`/`imul` pairs, then:

| Address | Instruction | Meaning |
| ------- | ----------- | ------- |
| `0x18008F4E8` | `and r8, qword ptr [rsi + 0x160]` | `hash & mask` |
| `0x18008F4FD` | `shl r8, 4` | bucket stride 16 bytes |
| `0x18008F4EF` | `mov rax, qword ptr [rsi + 0x138]` | end-of-chain sentinel |
| `0x18008F4F6` | `mov rdx, qword ptr [rsi + 0x148]` | bucket array |
| `0x18008F50B` | `cmp rbx, qword ptr [rcx + 0x10]` | compare the full `uint64` key |

So the accessor signature is `(BasicConfig*, uint64_t Tag)`. The "numeric ids" in
`_settings.tsv` are the *observed values* of those tags in this build; the type is 64 bits
and the values happen to be small. Calling them "small integers" is a description of this
build's data, not of the interface, and any reconstruction that types them as `int` would
break on a tag above `0x7FFFFFFF`.

### Two maps on the registry object

| Field | Map | Node layout |
| ----- | --- | ----------- |
| `+0x18` | `uint64_t Tag` → key name (`std::string`) | `+0x08` next, `+0x10` tag, `+0x18` buffer, `+0x28` size, `+0x30` capacity |
| `+0x58` | key name → `uint64_t Tag` | same shape, mirrored |
| `+0x98` | set of known tags | `+0x08`/`+0x10` red-black tree links, `+0x20` key |

Offsets above are relative to `FUN_18008A510()`, which is the global at `0x180800F58` plus
`0xD8`. The object is at least `0x4A8` bytes and lives in `.data`'s zero-fill tail, so none
of these fields has a file backing and every one reads as 0 before load-time construction.

Offsets from different bases are not comparable, so everything translated onto the object
itself. **This is the only form in which the triples can be checked against each other**,
and getting it wrong is exactly what produced the two false claims this file has corrected:

| Reader | Base | Field | Object offset |
| ------ | ---- | ----- | ------------: |
| `FUN_18002EE40` (`Bytes`) | `+0xD8` | buckets | `+0x198` |
| | | mask | `+0x1B0` |
| | | `+0xB0` | `+0x188` |
| `FUN_18008F420` (`bool`) | `+0xD8` | sentinel / buckets / mask | `+0x210` / `+0x220` / `+0x238` |
| `FUN_180076EB0` (serialiser) | `+0x128` | `+0xB0` | **`+0x1D8`**, a circular list head |

Offsets above are relative to `FUN_18008A510()` for the accessors and to the virtual `this`
for the serialiser. **They are not comparable as written** — translate onto the object
before comparing, which is the mistake this file has made twice and corrected twice. On the
object, `FUN_18008F420`'s triple lands entirely outside the constructor's initialised group at
`+0x210`/`+0x220`/`+0x238`, which is consistent: an empty map needs no constructor write.

## `keyFor`: the tag → name direction exists and is exported

- **VA** `0x180072E20`, RVA `0x72E20`, 41 bytes, 1 caller.
- **Tool** — `recovered/functions/180072e20_keyFor.c`, Ghidra signature:

```
basic_string<char,...> __cdecl keyFor(Tag param_1)
/* hackpro::extensions::keyFor(class hackpro::Tag) */
?keyFor@extensions@hackpro@@YA?AV?$basic_string@...VTag@2@@Z
```

It calls `FUN_18008A510` for the global registry, then `FUN_18007CCF0`, which is the name-map
lookup. On a miss `FUN_18007CCF0` logs `"CONFIG" … "{} not valid tag"`.

This settles an open question from `hackpro-api.md`: the exported API's "interned `Tag`
identity" is a 64-bit handle whose **name lives only in a runtime map**, and `keyFor` is the
supported way back from handle to name. There is no image-resident table of tag names,
which is the same negative result `notes/fnv-hash-probe.md` reached from the other
direction, and the two agree.

The named `hackpro::extensions` API block runs `0x180072DF0`–`0x180075530` and includes
`keyFor` (`0x180072E20`), `setHackEnabled` (`0x180073070`), `registerHackListener`
(`0x1800730F0`), the five `registerListener_<type>` overloads, and
`setConfigVar<std::string>` / `setConfigVar<std::vector<unsigned int>>`.

## The by-tag setter, and the five value types

`FUN_18008B490(this, Tag, char)` at VA `0x18008B490`, RVA `0x8B490`, 14 callers including
the exported `setHackEnabled` and `disableAllCheats` (`0x18019B450`). It is the
"boolean setter that intercepts settings by tag" from `hackpro-api.md`, now located:

| Call | Message |
| ---- | ------- |
| `FUN_18008DD20` | change notification |
| failure when the tag has a rule | `"Hack {} locked by rule"` |
| failure when the tag is absent | `"Invalid tag {} for mod setter"` |

The value types are the five group keys the whole-registry serialiser
`FUN_180076EB0` (`BasicConfig::vtable[0]`, 3,391 bytes, 1 caller `0x18008A530`) writes.
It walks the entry list at `this+0xB0`, calls `FUN_18007CCF0` per entry to recover the
name, and groups the output under one key per type:

| Type byte | Group key | Immediate in `FUN_180076EB0` |
| --------- | --------- | ----------------------------- |
| `\0` | `V_BOOL` | `0x4c4f4f425f56`, len 6 |
| `\1` | `V_INT` | `0x544e495f56`, len 5 |
| `\2` | `V_DECIMAL` | `movabs rax, 0x414d494345445f56` + `mov byte ptr, 0x4c`, len 9 |
| `\3` | `V_STRING` | `0x474e495254535f56`, len 8 |
| `\4` | `V_BYTES` | `0x53455459425f56`, len 7 |

Four of the five decode from their little-endian immediate alone
(`0x4c4f4f425f56` = `"V_BOOL"`). The fifth does not appear as one immediate, because
MSVC split it across a `movabs` and a single-byte tail store. `pe_disasm.py` at VA
`0x180077403`, RVA `0x77403`, file `0x767F0`:

```
0000000180077403  48 c7 45 b8 00 00 00 00   mov qword ptr [rbp - 0x48], 0
000000018007740b  48 c7 45 c0 09 00 00 00   mov qword ptr [rbp - 0x40], 9      ; size
0000000180077413  48 c7 45 c8 0f 00 00 00   mov qword ptr [rbp - 0x38], 0xf    ; capacity
000000018007741b  48 b8 56 5f 44 45 43 49 4d 41   movabs rax, 0x414d494345445f56
0000000180077425  48 89 45 b0             mov qword ptr [rbp - 0x50], rax
0000000180077429  c6 45 b8 4c             mov byte ptr [rbp - 0x48], 0x4c     ; 'L'
```

Size 9, eight bytes `V_DECIMA`, then `0x4C` — **`V_DECIMAL`**. Ghidra renders the
`mov byte` as `plStack_c0 = (longlong *)0x4c` and the `movabs` as `&DAT_414d494345445f56`,
which is why the decompiled form looked like a lost pointer. It is a lost *byte store*, and
`scripts/decode_sso.py` has no rule for single-byte tail stores — open work, and the reason
it recovers only three of these five from the recovered tree.

Type 2 is confirmed twice over, not just by the name: the entry loop at `0x1800774D6`
selects type `2` with `cmp byte ptr [r13 + 0x38], 2` and then loads its value with
`movsd xmm0, qword ptr [r13 + 0x18]` — an IEEE double. That agrees with the exported
`getConfigVar<double>` and `registerListener<double>`.

The five types agree with the five value types named independently in `hackpro-api.md`.

Consequence for M1: **the on-disk settings file is keyed by tag name, not by number.**
Any reconstruction of save/load has to reproduce the `keyFor` name map, not just the
values, or the file it writes will not be the file v9.1.3 reads.

## The insert, and where names come from

- **Date** — 2026-10-18.
- **Method** — `pe_exports.py`, then `_callers.tsv` walked from the exported `tagFor`, then
  the recovered `.c` for each link. No pattern-grepping; each step is a named edge in
  `_callers.tsv`.

### `tagFor` is the bridge M1 was looking for

`pe_exports.py` lists 30 named exports. One of them closes the gap the string inventory
named:

```
0x0000000180072df0  ?tagFor@extensions@hackpro@@YA?AV?$optional@VTag@hackpro@@@std@@AEBVbasic_string@@@Z
0x0000000180072e20  ?keyFor@extensions@hackpro@@YA?AVbasic_string@@AEBVTag@2@@Z
```

`hackpro::extensions::tagFor(std::string const&) -> std::optional<hackpro::Tag>`,
VA `0x180072DF0`, RVA `0x72DF0`, 41 bytes. It calls `FUN_18008A510` for the registry then
`FUN_18007CB60` — so `FUN_18007CB60` is `tagFor`'s worker, i.e. **the name → `Tag`
lookup**, returning a `std::optional` that is empty when the name is unknown.

`_callers.tsv` then settles what `FUN_18007CB60` is for. Its 40 callers include **every
exported config API**:

| Exported API | VA |
| ------------ | -- |
| `tagFor` | `0x180072DF0` |
| `registerActionListener` | `0x180072E80` |
| `isHackEnabled` | `0x180072FF0` |
| `setHackEnabled` | `0x180073070` |
| `registerHackListener` | `0x1800730F0` |
| `registerListener<bool>` … `registerListener<std::vector<unsigned>>` | `0x180073260`–`0x180074D70` |
| `setConfigVar<bool>` … `setConfigVar<std::vector<unsigned>>` | `0x180073800`–`0x180075310` |
| `getConfigVar<bool>` … `getConfigVar<std::vector<unsigned>>` | `0x180073880`–`0x180075430` |

So the public surface is **string-in, `Tag`-internally**: every `getConfigVar<T>(name)` and
`setConfigVar<T>(name, value)` resolves its `std::string` to a `Tag` first, then calls the
numeric accessors. This is the specific gap `notes/string-inventory.md` records — the UI
widget names and the numeric settings sharing no decompiled function. They are joined at the
**exported API boundary**, by `tagFor`, and the join is invisible to a per-function string
xref because the name arrives as a caller-supplied `std::string`, not a literal.

`FUN_18008A510` (VA `0x18008A510`, 21 bytes, **208 callers**) is the global registry accessor
they all share, calling `FUN_1800A2720` then `FUN_1800A2F90`.

### `internTag`: `FUN_18007C700`

`FUN_18007C700(registry, uint64_t Tag, const std::string* name)`, VA `0x18007C700`, RVA
`0x7C700`, 957 bytes, 5 callers. Read from `recovered/functions/18007c700_FUN_18007c700.c`:

1. Look up `Tag` in the tag→name map (`+0x20` sentinel, `+0x30` buckets, `+0x48` mask).
   If already present → log `"Key exists for tag {}"`.
2. Otherwise hash the **name** with FNV-1a-64 — a 4-bytes-per-iteration `imul` loop plus a
   `& 3` tail, unlike the straight 8-byte unroll used for `Tag` keys — then look up the name
   in the name→tag map (`+0x60`, `+0x70`, `+0x88`). If present → log `"Tag exists for key {}"`.
3. Otherwise insert into **both**, via `FUN_1800810F0(param_1 + 0x18, …)` keyed by tag and
   `FUN_180081720(param_1 + 0x58, …)` keyed by name.

Duplicate detection in both directions means tag ↔ name is a **bijection**. That is the
structural fact a reconstruction depends on: no two names share a tag, and no two tags
share a name.

So the two maps sit at `+0x18` and `+0x58`; the note's `+0x20`/`+0x30`/`+0x48` and
`+0x60`/`+0x70`/`+0x88` are the inner fields of those map objects, 8 bytes in.

### The names are data, not code: `hackpro.json`

`FUN_18007C700`'s five callers are `FUN_180003E340`, `FUN_180085DE0`, `FUN_18008B780`,
`FUN_18008DF30` and `FUN_1800B8B40`. `FUN_18008B780` is the one that matters:

- VA `0x18008B780`, RVA `0x8B780`, 4,763 bytes, 1 caller.
- It is a **nlohmann/json loader**. Messages recovered verbatim from its literals:
  `"stream pointer null"`, `"config json null"`, `"config json not object"`,
  `"object {} doesn't contain valid 'tag' key"`, `"tag {} already exists in config"`,
  `"object {} doesn't contain 'default' key"`, `"OPTION {} has invalid type for key
  'default'"`, `"type must be boolean, but is "`.
- It iterates the parsed JSON object; for each member it reads the **`"tag"`** key, requires
  it to be a JSON number (`*pcVar7 - 5 <= 1`, i.e. nlohmann `number_integer`/`number_unsigned`),
  converts it to a `uint64`, and calls `FUN_18007C700(registry, tag, member_key)`. The
  member's **JSON key is the setting's name**; the `"tag"` value is the number.
- Optional per-setting keys: `"action"` (bool) and **`"nosave"` (bool)**, the latter routing
  to `FUN_18007D090(registry, tag)`.
- Defaults are applied through the per-type setters, dispatching on the default's JSON type:
  `FUN_180041D60`, `FUN_180040FC0`, `FUN_180042060`, `FUN_180086AD0`, `FUN_1800867B0`.

Where the stream comes from: `_xrefs.tsv` has `hackpro.json` at VA `0x180547868` with exactly
two referencing functions, `FUN_1800BF190` and `FUN_1800BF400`. `FUN_1800BF190` (VA
`0x1800BF190`) builds the mod directory with `FUN_1800BEAA0`, appends `"hackpro.json"`, and
constructs a `std::ifstream` (shared_ptr, `0x120` bytes, refcount `0x100000001`). Both have
**zero direct callers** — reached through a pointer, so the direct-call graph cannot see
them.

The chain, every link a row in `_callers.tsv`:

```
FUN_180002320   mod bootstrap: the two tabs (registerTab x2) + hackpro singleton init
  └─ FUN_1800A2FB0   open and parse the config
       └─ FUN_1800BF190   open hackpro.json
       └─ FUN_18008B780   parse, validate, intern each tag
            └─ FUN_18007C700   internTag
```

`FUN_180002320` (VA `0x180002320`, 537 bytes, 3 callers) is the same function that
[tab-registration.md](tab-registration.md) identifies as registering Mega Hack's two tabs,
which independently corroborates that note.

**This is the answer to the biggest open question in this file.** The complete registry —
every key, its numeric tag, its type, its default and whether it is saved — is **not in the
binary at all**. It lives in `hackpro.json` in the installed Geode mod folder. A static
analysis of `_original.dll` cannot enumerate it, which is why the registry looked
unrecoverable and why `_settings.tsv`'s 134 tags are a sample of what code happens to read
rather than a count of what exists.

Per `AGENTS.md`, that file is user data and is **not** read into or committed from this
repository. Its schema is recorded; its contents are not.

`FUN_180076EB0` (the serialiser) is the inverse: it walks the entry list at `this+0xB0` and
writes each entry out under `FUN_18007CCF0`'s recovered name, so save and load agree on
names and the `"nosave"` flag is what keeps an entry out of the file.

### Two separate on-disk stores, not one

Worth stating plainly, because "Mega Hack's save file" is ambiguous and the two are easy to
conflate.

**hackpro's own store** is `hackpro.json` in the mod directory. Read by `FUN_1800BF190`
(VA `0x1800BF190`, `std::ifstream`, refcount object `0x120` bytes) and written by
`FUN_1800BF400` (VA `0x1800BF400`, `std::ofstream`, refcount object `0x118` bytes). The two
are near-identical (302 bytes each) and differ in the open mode: the reader calls
`FUN_1800423F0(…, 1, 0x40, 1)` and the writer `FUN_180036B20(…, 2, 0x40, 1)`. Both build
the path with `FUN_1800BEA70`/`FUN_1800BEAA0` and append via `FUN_1800BF320`. Both have
**zero direct callers** — dispatched indirectly, so `_callers.tsv` cannot see them.

**Geometry Dash's own save files** are read separately, by `FUN_180042FB0` (VA
`0x180042FB0`, RVA `0x42FB0`, 1,587 bytes, 1 caller). It:

1. calls `FUN_1800C5210` to obtain a base directory;
2. bails with the language key `ERROR/HOME/NO_APPDATA` (VA `0x18053FB8F`) if it is empty;
3. otherwise appends the literal `Octavus` (VA `0x18053FBA5`) via `FUN_180044C60`;
4. reads `data.json` into its second argument and `save.json` into its third — both via
   `FUN_1800423F0(…, 1, 0x40, 1)`, i.e. the same reader shape as `hackpro.json`.

`FUN_1800C5210` (VA `0x1800C5210`, 98 bytes) is the base-directory resolver, and it is
unambiguous:

```
iVar1 = SHGetKnownFolderPath(&DAT_1805820e0, 0, 0, &local_10);
```

The 16 bytes at VA `0x1805820E0` (RVA `0x5820E0`, file `0x5814E0`) read back as
`f1b32785-6fba-4fcf-9d55-7b8e7f157091`, which is **`FOLDERID_LocalAppData`** —
`%LOCALAPPDATA%`. So Mega Hack reads GD's `data.json` and `save.json` from a subdirectory of
the user's local app data.

`Octavus` is Absolllute's own directory name, so the chain is
`%LOCALAPPDATA%\<sub>\Octavus\{data.json,save.json}`.

(Resolved 2026-10-20.) `<sub>` is empty: `FUN_1800C5210` calls `SHGetKnownFolderPath`
with `FOLDERID_LocalAppData`, and `FUN_180042FB0` appends `Octavus` directly, so the path is
`%LOCALAPPDATA%\Octavus\{data.json,save.json}`. See notes/persistence.md. `_xrefs.tsv`
contains **no** reference to the literal `GeometryDash` anywhere, so the `GeometryDash`
directory is not hard-coded as a string in this binary — it is either supplied by Geode's
directory abstraction or derived at runtime. Confirming it needs the Geode host contract for
save-data paths; see [references/](references/).

These two stores are the reason M4's `best_runs.bin` work is a third, separate mechanism
rather than an extension of either.

## Conclusion

1. The registry lives on `hackpro::BasicConfig`; `hackpro::Config` derives from it at
   `+0x128` and its `vtable[2]` is a `getConfigVar<bool>` forwarder.
2. Settings are keyed by **`hackpro::Tag`, a 64-bit value**, not by a small number. The
   134 values in `_settings.tsv` are what this build uses, not the interface's type.
3. Tag names exist only in a runtime map on the registry, and tag ↔ name is a **bijection**
   maintained by `FUN_18007C700`.
4. **The whole registry is `hackpro.json`, not the binary.** `FUN_18008B780` reads each
   member's JSON key as the name and its `"tag"` number as the `Tag`, then interns the pair.
   No static analysis of `_original.dll` can enumerate it. This closes M1's "where are the
   names" question by relocating it: the answer is in the shipped data, which is
   deliberately not committed.
5. The public API is **string-in**: every exported `getConfigVar<T>`/`setConfigVar<T>` takes a
   `std::string`, resolves it with `tagFor` (`0x180072DF0`) to a `Tag`, then uses the
   numeric accessors. That is the bridge between the UI widget names and the numeric
   settings, and it is invisible to per-function string xrefs because the names arrive as
   caller arguments rather than literals.
6. Five value types, written as `V_BOOL`, `V_INT`, `V_DECIMAL`, `V_STRING`, `V_BYTES`
   groups by `FUN_180076EB0`, agreeing with `hackpro-api.md`'s five. `V_DECIMAL` is
   confirmed by disassembly (a `movabs` plus a `0x4C` byte store, size 9) and type 2 is
   confirmed independently by the `movsd` that loads its value.
7. The save file is keyed by **name**, so save/load parity requires reproducing the name
   map and honouring `"nosave"`.
8. There are **two** on-disk stores, not one: hackpro's `hackpro.json` in the mod directory
   (read `0x1800BF190`, written `0x1800BF400`), and GD's own `data.json` / `save.json` under
   `%LOCALAPPDATA%\<sub>\Octavus\` (read by `0x180042FB0`; `%LOCALAPPDATA%` confirmed from
   the `FOLDERID` GUID at `0x1805820E0`). M4's `best_runs.bin` is a third mechanism.
9. All seven ids previously listed as unknown are located and their roles partly identified.
10. Ids 501/502 and 445/446/447 are read as groups, which is where a session/global switch
    would most plausibly sit.
11. Any index built from an assumed accessor list undercounts. Enumerate, do not assume.

## Open questions

- (Partially resolved 2026-10-20.) The `"action"` flag is a boolean in the JSON entry; when
  it is true, `FUN_18008B780` requires the entry to be an object and jumps to
  `LAB_18008C1EC` (the action-registration path). When false, the entry follows the normal
  typed-value path. UNVERIFIED: the exact consequence of the action path (whether it
  registers a UI action instead of a config value).
- (Unchanged 2026-10-20.) `scripts/decode_sso.py` still has no rule for MSVC's
  single-byte tail stores (`mov byte ptr [rbp - 0x48], 0x4c`), so it recovers only three of
  the five `V_*` group keys. `pe_disasm.py` closes the gap case by case. UNVERIFIED: a
  general rule; deferred to a future decoder pass. See [sso-decoder.md](sso-decoder.md).
- (Unchanged 2026-10-20.) `FUN_18008A510`'s 208 callers all take the same registry, so
  nothing in the binary bounds the tag space; whether `hackpro.json` and the mod's UI agree
  on the full list is a question about the data, not the binary. UNVERIFIED: the full key
  list (data, not binary).
- (Resolved 2026-10-20.) The ICONIC row ids (`445`, `446`, `447`, `0x1BD`-`0x1BF` in
  `FUN_18011FFE0`) are **UI definition ids**, not settings tags: they are passed to the
  UI-row helpers `FUN_180120A40`/`FUN_180120B70` in the same function, never to
  `FUN_18008F420`/`FUN_18002DE50`/`FUN_180003D80`/`FUN_18002EE40`. They share only the
  numeric range by coincidence; no tag in `_settings.tsv` is `0x1BD`-`0x1BF`. (Resolved
  2026-10-20.) The enumerated `FUN_180120A40` row ids (32, 394-399, 402, 411, 414, 422,
  432-434, 436, 442, 445, 448) are sparse, so the UI id sequence has gaps; it is not a
  dense 0..N range.

## Failure modes recorded

- **Comparing struct offsets read through different `this` bases.** This produced **two**
  false claims in this file. The first was "`FUN_18002EE40` is a second registry", from its
  offsets differing from `getConfigVar<bool>`'s. The second was "`+0xB0` is claimed twice",
  from the serialiser and the accessor both writing `+0xB0`. In both cases the offsets were
  read through differently-based pointers — one via `FUN_18008A510()` (`object + 0xD8`), one
  via virtual dispatch (`object + 0x128`) — and neither was the same field.
  **Offsets are only comparable once translated onto a common base**, and the base has to be
  derived, not assumed from the fact that both functions talk about "the config".
- **Grepping for an offset across the whole tree is worse than useless.** `+0xB0` appears 385
  times in 280 files. Filtering to functions that actually receive the registry reduces that
  to **two**, neither of which is the pair in question. Always filter by *receiver* first,
  then by offset.
- **A guessed accessor list produced a plausible table that silently omitted setting 501.**
  The omission was only visible because a hand-recorded id from an earlier note failed to
  appear. Cross-check index output against ids already known from other notes.
- **Guessing *argument position* cost tag 285, in exactly the same way.** The matcher took
  "the first comma-separated argument that looks numeric"; `FUN_18002EE40` puts its tag
  third. Fixed by recording the index per accessor, and now pinned by
  `scripts/test_settings_index.py`.
- **That test was verified to fail before it was trusted.** Both bug classes were
  re-injected deliberately: setting the accessor index back to 1 reproduces the original
  failure exactly (tag count drops 149 → 134, eleven assertions fail), and replacing the
  nesting-aware split with a naive `split(",")` fails the two comma-in-a-cast checks. A test
  that has never been seen to fail is not evidence of anything.
- **Indexing `&local_XX` as if it were a key name would have produced four nonsense
  entries.** It is better to report the local than to invent a name.
- The splitter must track nesting: a real call is
  `FUN_18002ee40(param_1,(vector<unsigned_int,std::allocator<unsigned_int>_> *)…)`, whose
  cast contains a comma. A naive split puts the tag one position later and reads it as
  `None`.