# The string inventory

- **Question** — M1 asks for the binary's strings split into user-visible labels, setting
  keys, file paths, language keys and format strings, each anchored to a VA. What is there,
  and what does each group actually contain?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/classify_strings.py` → `recovered/functions/_strings.tsv`.

## Where the strings come from

`_xrefs.tsv` already carries 4,145 code-referenced strings, 3,609 of them anchored to a VA
in `.rdata`/`.data`. The remaining **536 are SSO-derived and carry no VA**, because those
bytes only ever exist inside a stack frame; they are classified and flagged `sso` rather than
promoted, since they inherit the split and merge error modes in [sso-decoder.md](sso-decoder.md).

## The split

| Category | Rows | Contains |
| -------- | ---: | -------- |
| `other` | 1,079 | see "what is still missing" below |
| `mod-identifier` | 974 | Mega Hack's own snake_case names — `icons_0`, `override_cols`, `player_ball`, `no_bar`, `same_dual_col` |
| `user-label` | 479 | UI text |
| `format-string` | 329 | printf (`%s`, `%*s`) and `{}` / `{:02}` templates |
| `third-party` | 272 | statically linked OpenSSL/zlib, gated on a known prefix |
| `host-method` | 214 | `PlayLayer::createCheckpoint`, `cocos2d::CCDirector::popScene` |
| `library-text` | 212 | lowercase library diagnostics — libc++, BoringSSL, libpng, zlib, nlohmann/json |
| `flag` | 153 | `HACK_COPY_HACK+` and similar |
| `setting-key` | 126 | `ICONIC/DISABLE`, `HOME/THEME/{}/COL` |
| `mangled-symbol` | 96 | MSVC `?f@ns@@` and `??1Class` forms |
| `file-path` | 108 | asset and data filenames, including `.plist`, `.dll` and `{}` templates |
| `ui-element` | 77 | Mega Hack's own widget names |
| `error-text` | 12 | assertion and diagnostic strings |
| `url` | 6 | including `https://absolllute.com/store/mega_hack` |
| `api-path` | 4 | `/api/mega_hack/...` |
| `language-key` | 4 | |

Rules are **ordered and first-match-wins**, so the order is part of the finding: a
setting key that also looks like a path lands in `setting-key`.

## The interesting group: `ui-element`

77 strings name Mega Hack's own interface widgets — `CHECKBOX_HIDE_INTERFACE`,
`CHECKBOX_JUMPSCARE_ONLY_0`, `CHECKBOX_LEGACY_CALCULATION`, `COLOUR_PASSABLE_HITBOX`, and
similar. Nothing in the notes had these, and they are a direct lead into M3's feature map:
each is a named control that exists in the settings UI. UNVERIFIED: whether every name here
corresponds to a live control, and what the `CHECKBOX_` prefix distinguishes from `COLOUR_`.

## A regression worth recording

The first attempt filled the `third-party` bucket with a generic snake_case rule, which
looked like a clean 434-row gain. Checking the bucket contents showed **974 of its 1,246 rows
matched no OpenSSL prefix at all** — they were Mega Hack's own identifiers. The bucket was
78% wrong while looking tidy.

Generic snake_case now has its own `mod-identifier` category, and `third-party` requires a
known library prefix. That split is honest but not clean: `mod-identifier` still contains
OpenSSL names the prefix list misses — `dhx_to_PrivateKeyInfo_der_encode`,
`evp_mac_from_algorithm`, `rsa_signverify_init`. Resolving that needs the OpenSSL symbol
table, not more prefix guessing.

The general lesson, which has now cost real rework twice in this repo: **check what is in a
bucket, not how big it is.** A category that looks tidier than the data is a category that
is lying.

## The UI names and the numeric settings never meet

Joining the 77 `ui-element` names against the 138 numeric settings in `_settings.tsv`, by
shared decompiled function, gives **zero overlaps**. Not one UI control name and one numeric
setting id are read by the same function.

That is a useful negative. It means the two key spaces are not connected the way the obvious
join assumes:

- **Numeric ids** — 138 of them, read through `FUN_18008F420`, `FUN_18002DE50` and
  `FUN_180003D80`. See [settings-registry.md](settings-registry.md).
- **UI control names** — the `CHECKBOX_*` / `COLOUR_*` strings, apparently read somewhere else
  entirely.

So Mega Hack appears to have **at least two key spaces** that are bridged somewhere this pass
has not found: a numeric id registry, and a string-keyed map behind `FUN_18007CB60`. The UI
names most likely go through the string side, and something translates between them.

UNVERIFIED: where the bridge is. The obvious candidate is `hackpro::Config`, which the RTTI
hierarchy shows deriving from both `hackpro::BasicConfig` and `hackpro::NodeDelegate` — a type
sitting across both worlds. That is a lead, not a finding, and no code path was traced.

This is exactly the gap M1's settings criterion names: "every key, its type, default, and the
function that applies it." The two key spaces are now known to be separate, and the join
between them is the specific thing still missing.

## The UI table: one function, 16 KB, no callers

All 77 `ui-element` names are referenced from a **single** decompiled function:

| | |
| --- | --- |
| Address | `FUN_180062940` (VA `0x180062940`, RVA `403776`) |
| Size | **16,037 bytes** |
| Callers | **0** |
| Settings accessors called | none |

A 16 KB function with no callers that names every control in the interface is a **static
registration table** — the place Mega Hack builds its settings UI. It is the anchor M3's
feature map was missing.

It calls none of the three numeric accessors (`FUN_18008F420`, `FUN_18002DE50`,
`FUN_180003D80`) and none of `FUN_18007CB60`. That is the same absence the join found, now
localised to one function: **the UI is built here and the settings are read elsewhere**, and
whatever connects them is a third thing this pass has not located.

It is **confirmed a single function**, not a Ghidra merge:

| Check | Result |
| ----- | ------ |
| Other indexed functions starting inside the range | 0 |
| `return` statements in the body | 2 |
| **`.pdata` runtime-function entries** | **exactly 1**, `0x180062940`–`0x1800667EA`, 16,042 bytes |

`.pdata` is the authoritative record of x64 function boundaries — the OS walks it for
exception handling — so a single entry settles it. The earlier caveat about several functions
merged is withdrawn: it was worth checking, and it is not the case.

Its zero callers are **explained**, though the first answer was wrong and is recorded below
rather than quietly replaced.

### It is a CRT static-initializer entry, not a vtable slot

The function's VA occurs exactly once in the image, at `0x18063F0C8`, inside a **281-entry
table of `.text` function pointers** at `.rdata` `0x18063F070`–`0x18063F938`:

| Property | Value |
| -------- | ----- |
| Table start (`rcx`) | `0x18063F068` — holds `0` |
| First entry | `0x18063F070` = `0x1803A50A8` |
| `FUN_180062940` | entry **11** of 281, at `0x18063F0C8` |
| Last entry | `0x18063F930` = `0x18027EDD0` |
| Table end (`rdx`) | `0x18063F938` — one past the last entry |
| Sibling table | `0x18063F940`–`0x18063F948`, **all zero** |

The table has exactly **one** RIP-relative reference in the entire image, at `0x180531213`
inside `FUN_18053118c`:

```asm
0x180531213  lea  rdx, [rip + 0x10e71e]   ; 0x18063F938  one past last entry
0x18053121a  lea  rcx, [rip + 0x10de47]   ; 0x18063F068  start (holds 0)
0x180531221  call 0x180533e10             ; _initterm
```

`rcx`/`rdx` as a begin/last pair is the MSVC x64 range convention, so this iterates the
table and calls each entry **indirectly**. That is the whole reason Ghidra attributed zero
callers: the function is invoked through a pointer, never by a direct `call`.

**The callee is confirmed, not inferred.** `0x180533e10` is a 6-byte IAT jump thunk
(`jmp qword ptr [rip + 0x110CEA]`, INT3-padded), and the slot resolves to an import:

| Thunk | IAT slot | Import |
| ----- | -------- | ------ |
| `0x180533E10` | `0x180644B00` | `api-ms-win-crt-runtime-l1-1-0.dll!_initterm` |
| `0x180533E20` | `0x180644B08` | `api-ms-win-crt-runtime-l1-1-0.dll!_initterm_e` |
| `0x180533E30` | `0x180644B10` | `api-ms-win-crt-runtime-l1-1-0.dll!_register_onexit_function` |

So `FUN_18053118c` is CRT startup code, and the 281-entry table is the **`_initterm` static
initializer table**. The neighbouring `_initterm_e` range at `0x18063F940`–`0x18063F948` is
**entirely zero** — an empty C++-destructor range — so all 281 initializers run through the
C-style table. `FUN_180062940` is therefore a **DLL-load static constructor**, and the 77 UI
names it carries are registered at load time, not from any call site in Mega Hack's own code.
That is a feature-mapping fact for M3, and it is why the function must not be treated as
orphaned.

Ghidra never created functions for these thunks, which is why `0x180533E10` is absent from
`_index.tsv` — a gap in the index, not an absence of code.

### The first answer, and why it was wrong

The initial guess was a **vtable slot**, on the reasonable-sounding grounds that the pointer
sits in `.rdata` between two code addresses. `rtti_index.py` contradicts that directly:

- `0x18063F0C0` is in **none** of the 497 validated vtables.
- **No** vtable starts within ±`0x40` of it.
- There is no Complete Object Locator at `[-1]`.

Reading `vtable[-1]` returned `0x18003D1A0`, a `.text` address, and following it to a Type
Descriptor produced garbage. That garbage is not recorded as a finding. A 281-entry,
zero-terminated table with a single one-past-the-end reference is not a vtable; guessing from
the neighbours instead of running the validator cost a commit and a correction.

Sample of the names it wires up — this is a readable feature list:

```
CHECKBOX_DEAFEN_IN_PRACTICE     CHECKBOX_NOCLIP_ACCURACY_LIMIT
CHECKBOX_DEAFEN_WITH_STARTPOS   CHECKBOX_NOCLIP_DEATH_LIMIT
CHECKBOX_FILL_HITBOX            CHECKBOX_HIDE_INTERFACE
CHECKBOX_HAZARDS_HITBOX         CHECKBOX_HITBOXES_PLAYER
CHECKBOX_LEGACY_CALCULATION     CHECKBOX_JUMPSCARE_ONLY_0
COLOUR_PASSABLE_HITBOX
```

`CHECKBOX_DEAFEN_*` appears twice in the binary, so **"DEAFEN" is Mega Hack's own
spelling**, not a decoding artefact from `decode_sso.py`. It is likely a typo in the mod's own source, and is recorded because a reconstruction should
match it rather than silently correct it.

## What is still missing

`other` was 1,359 rows — 33% of the inventory. A second pass brought it to **1,079 rows,
26%**, by fixing three concrete rule bugs rather than adding broad heuristics:

| Change | Rows moved | Why it was a bug, not a judgement call |
| ------ | ---------: | ------------------------------------- |
| `library-text` (new) | 212 | lowercase diagnostic prose with a space |
| `file-path`: added `.dll`/`.so`, allowed `{}`, `:`, `,` | 22 | `libcocos2d.dll` and `player_{:02}.png` failed on characters the pattern forbade |
| `format-string`: allowed `*` in the flag class | 34 | OpenSSL prompts are `%*sPath Length Constraint:` |

`library-text` was measured before it was added: 305 rows in `other` matched a loose
"lowercase with a space" shape, and 35 were sampled and read. They are library diagnostics, not
unknown text — libc++ (`bad array new length`, `iterator out of range`), BoringSSL
(`m_cost must be greater or equal than 8 times the number of lanes`), libpng
(`malformed sPLT chunk`), nlohmann/json (`syntax error while parsing`), zlib
(`internal write transform logic error`), c-ares (`calling fread()`). Mega Hack's own labels are
capitalised and already matched by `NATURAL_LABEL`, so the rule does not take MH's text; 12
post-change samples were checked to confirm that.

The 1,079 that remain are now a **characterised** remainder, not an unknown one:

| Population | Example | Why it is left alone |
| ---------- | ------- | -------------------- |
| OpenSSL ASN.1 field fragments | `gerprint`, `xcghash`, `ld-type`, `use-bits` | bare lowercase, indistinguishable from short words by shape |
| Flat uppercase identifiers | `SHORTCUT_FRAME_STEPPER_PAUSE`, `SHORTCUT_STARTPOS_L` | no `/` segment, so `setting-key` misses them; a `^[A-Z][A-Z0-9_]+$` rule would also take every OpenSSL macro |
| SSO decoder artefacts | `urcolo`, `gBur`, `onopti`, `libcocos2d.dll<?` | 181 rows, and these are the decoder's documented split/merge error modes, not strings |

M1's criterion asks for the inventory to be **split**, and a split that leaves a quarter of it
in one bucket does not meet it. The criterion therefore stays unchecked, and the number is left
visible rather than tuned away.

The ASN.1 fragments and the SSO artefacts both have a *known* fix that is not a regex: resolve
field names against the OpenSSL symbol table, and mark SSO rows whose shape matches a documented
decoder error instead of classifying them at all.

## Conclusion

1. 4,145 strings classified into 17 categories, 3,609 VA-anchored and 536 SSO-only.
2. Mega Hack's own widget names are recoverable, which is a direct M3 lead.
3. `other` is down to 26% from 33%, and what remains in it is characterised rather than unknown.
4. The largest recovered group after `other` is Mega Hack's own 974 identifiers, not
   third-party code — which corrects the impression that this binary is mostly OpenSSL.

## Open questions

- Resolve `mod-identifier` against the OpenSSL symbol table to split Mega Hack's names from
  the library's.
- Give the 181 SSO artefacts a shape-based flag tied to the decoder's documented error modes,
  rather than leaving them in `other` to distort the percentage.
- Confirm whether every `ui-element` name maps to a live control, and extend that group into
  the M3 feature map.
- Separately: `/api/mega_hack/...` and the licence strings are **out of scope** — see
  [not-investigated-licence-path.md](not-investigated-licence-path.md). They appear here only
  because they are strings in the binary, and nothing in this note maps their behaviour.