# The string inventory

- **Question** — M1 asks for the binary's strings split into user-visible labels, setting
  keys, file paths, language keys and format strings, each anchored to a VA. What is there,
  and what does each group actually contain?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/classify_strings.py` → `recovered/functions/_strings.tsv`.

## Where the strings come from

`_xrefs.tsv` already carries 4,178 code-referenced strings, 3,611 of them anchored to a VA
in `.rdata`/`.data`. The remaining **567 are SSO-derived and carry no VA**, because those
bytes only ever exist inside a stack frame; they are classified and flagged `sso` rather than
promoted, since they inherit the split and merge error modes in [sso-decoder.md](sso-decoder.md).

`scripts/classify_strings.py` classifies **4,175** of those rows — three whitespace-only rows
are dropped — leaving **3,609 VA-anchored and 566 SSO-only**. Both numbers are what the
classifier prints; the difference from `_xrefs.tsv` is the three empty strings, not a lost row.

## The split

| Category | Rows | Contains |
| -------- | ---: | -------- |
| `third-party` | 2253 | statically linked OpenSSL/zlib: prefix-gated names **plus** every string whose referencing functions all live in the library code region (>= 0x1803A4EE0) — the region rule now covers every Mega Hack-facing category, not just `mod-identifier`/`other` (see the 2026-10-21 update below) |
| `mod-identifier` | 23 | Mega Hack's own snake_case names — `icons_0`, `override_cols`, `player_ball`, `no_bar`, `same_dual_col` (plus 12 SSO artefacts; see below) |
| `other` | 353 | JSON keywords, CRT constants, version strings and SSO decoder artefacts (see "what is still missing") |
| `user-label` | 177 | UI text |
| `format-string` | 145 | printf (`%s`, `%*s`) and `{}` / `{:02}` templates |
| `library-text` | 219 | lowercase library diagnostics — libc++, BoringSSL, libpng, zlib, nlohmann/json |
| `host-method` | 214 | `PlayLayer::createCheckpoint`, `cocos2d::CCDirector::popScene` |
| `error-text` | 178 | assertion and diagnostic strings, exception templates |
| `setting-key` | 165 | `ICONIC/DISABLE`, `HOME/THEME/{}/COL`, `SHORTCUT_*`, `OPTIONS_*`, `V_*`, schema keywords |
| `flag` | 153 | `HACK_COPY_HACK+` and similar |
| `file-path` | 108 | asset and data filenames, including `.plist`, `.dll`, `.so`, `.ttc` |
| `mangled-symbol` | 96 | MSVC `?f@ns@@` and `??1Class` forms |
| `ui-element` | 77 | Mega Hack's own widget names |
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

Joining the 77 `ui-element` names against the 184 numeric settings in `_settings.tsv`, by
shared decompiled function, gives **zero overlaps**. Not one UI control name and one numeric
setting id are read by the same function.

That is a useful negative. It means the two key spaces are not connected the way the obvious
join assumes:

- **Numeric ids** — 184 of them, read through `FUN_18008F420`, `FUN_18002DE50`,
  `FUN_18002E4C0`, `FUN_180003D80`, `FUN_18002D710` and `FUN_18002EE40`. See
  [settings-registry.md](settings-registry.md).
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

It calls none of the six numeric accessors (`FUN_18008F420`, `FUN_18002DE50`,
`FUN_18002E4C0`, `FUN_180003D80`, `FUN_18002D710`, `FUN_18002EE40`) and none of
`FUN_18007CB60`. That is the same absence the join found, now
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

The 353 that remain (down from 942) are now a **characterised** remainder, not an unknown one:

| Population | Count | Example | Why it is left alone |
| ---------- | ----: | ------- | -------------------- |
| JSON / parser keywords used by MH code | ~138 | `list`, `object`, `array`, `string`, `null`, `value` | lowercase single tokens with no `/`; a broad rule would also take genuine identifiers; they are libc++/nlohmann tokens, not MH labels |
| CRT / constant tokens | ~70 | `0123456789ABCDEF`, `v9.1.3`, `nan(ind)` | not labels, keys, paths or language keys |
| SSO decoder artefacts | 142 | `urcolo`, `gBur`, `onopti`, `libcocos2d.dll<?` | these are the decoder's documented split/merge error modes, not strings |
| Flat uppercase identifiers | 5 | `ERROR_NULL`, `LOAD_WATCH`, ... | no `/` segment; `^[A-Z][A-Z0-9_]+$` would also take every OpenSSL macro |

M1's criterion asks for the inventory to be **split** into user-visible labels, setting keys,
file paths, language keys and format strings. Those five categories are now populated and
VA-anchored, and none of the 353 residual rows belongs to any of them — they are JSON
keywords, CRT constants and SSO artefacts. The residual is recorded here rather than tuned
away, per the note's own rule.

The ASN.1 fragments and the SSO artefacts both have a *known* fix that is not a regex: resolve
field names against the OpenSSL symbol table, and mark SSO rows whose shape matches a documented
decoder error instead of classifying them at all.

### A rejected approach: detecting SSO artefacts by text shape

The obvious cheap fix for the 181 SSO rows is to flag them by shape. Measured, and **not
adopted**, because precision is too low to be worth anything:

| Test | Matches | Verdict |
| ---- | ------: | ------- |
| SSO text is a suffix of any other inventory string | 37 | false merges with genuine strings |
| … requiring the longer string to be VA-backed (real image bytes) | 21 | still mostly false positives |
| SSO text is a substring of a longer string | 20 | **unsound, rejected outright** |

The substring test is wrong in principle: it flags `HACK_NO_CAMERA` because it is contained in
`HACK_NO_CAMERA_ZOOM`, and `libcocos2d.dll` because it is contained in `libcocos2d.dll%/` — both
genuine strings. The stricter suffix test is better but still imprecise, because ordinary words
are suffixes of longer identifiers: `ball` of `player_ball`, `hack` of `absolllute.megahack`,
`list` of `plist`, `string` of `cstring`, `label` of `ep-label`.

So text alone cannot separate a truncated literal from a real short word, and no rule is added.
The 181 rows stay where they are. The fix that *would* work is already recorded in
[sso-decoder.md](sso-decoder.md): recover the store layout precisely enough to fix the
split/merge modes, which needs the disassembly rather than the decompiler's local names. Shape
matching is the guess that rule would replace.

One incidental gain: the truncation test reproduces, from the data, three examples quoted in
[sso-decoder.md](sso-decoder.md) as known error modes — `/SET`, `/REMOVE` and `/VIEW` as
truncations of `KEYBINDS/SET`, `KEYBINDS/REMOVE` and `KEYBINDS/VIEW`. That is independent
confirmation of the documented mode, not a new finding.

## Conclusion

1. 4,175 strings classified into 16 categories, 3,609 VA-anchored and 566 SSO-only.
2. Mega Hack's own widget names are recoverable, which is a direct M3 lead.
3. `other` is down to **8.5% (353 rows)** from 26%, and what remains in it is characterised
   rather than unknown: JSON keywords, CRT constants, version strings and SSO decoder
   artefacts. None of those are Mega Hack labels, setting keys, paths or language keys.
4. The OpenSSL mass that previously polluted `mod-identifier` (974 of 980 rows) is now
   correctly attributed to `third-party` (2,253 rows) by **code-region evidence**, not
   prefix guessing. `mod-identifier` now holds only Mega Hack's own identifiers
   (`icons_0`, `override_cols`, `player_ball`, `no_bar`, `same_dual_col`, ...).
5. The split criterion for the five Mega Hack-relevant categories (user-visible labels,
   setting keys, file paths, language keys, format strings) is now met: each is populated
   and VA-anchored, and the residual `other` bucket contains no strings belonging to them.

## Update 2026-10-21 — region-based reclassification

The open question in this note was whether `mod-identifier`'s 980 rows were Mega Hack's own
names or OpenSSL's. Prefix guessing was tried and rejected (78% wrong). The deciding
evidence is **where the referencing code lives**, not what the string says:

- Recovered function filenames are address-prefixed (`180xxxx_FUN_...`). Their address
  distribution has a single large gap: `0x1802FFDA0 -> 0x180352640 -> 0x1803A4EE0`
  (0x528a0 bytes). Everything below `0x1803A4EE0` is Mega Hack's own code; everything
  at/above it is the statically-linked library (OpenSSL, zlib, ...).
- Every `mod-identifier` row referenced only by functions at/above `0x1803A4EE0` is
  therefore a library-internal name. 957 of the 980 rows qualify and moved to
  `third-party`. The 23 that remain are referenced by Mega Hack's own code
  (`0x1801xxxxx`) and are genuine identifiers.
- The same rule was applied to `other`: 548 VA-backed rows referenced only by the library
  region moved to `third-party`, taking `other` from 942 to 353.

Implementation: `scripts/classify_strings.py` now loads `recovered/functions/_xrefs.tsv`,
maps each VA (and each SSO text) to its referencing functions, and reclassifies a
`mod-identifier`/`other` row as `third-party` when **all** of those functions have a base
address >= `0x1803A4EE0`. Rows with no referencing function (or a mixed region) keep
their category. `classify(text, va)` gained the `va` parameter; the pure-text rules are
unchanged, so the **text** classification of the five MH-relevant categories is
unaffected. (The *region* promotion was later extended to those categories too — see
the next section.)

Verification:
- Genuine identifiers survived: `icons_0`, `icons_1`, `cols_0`, `cols_1`, `extra_0`,
  `extra_1`, `override_cols`, `glow_0`, `glow_1`, `same_dual_col`, `no_bar`,
  `player_ball` — all referenced by `0x18012ebe0` / `0x180132ce0` / `0x1802005a0`
  (Mega Hack's own code), so they stayed in `mod-identifier`.
- The moved rows are OpenSSL symbol names (`asn1_cb`, `bn_expand_internal`,
  `dhx_to_PrivateKeyInfo_der_encode`, ...), which is what the strings actually are.
- `test_pe_rva.py`, `test_settings_index.py`, `test_init_table.py`,
  `test_progress_svg.py` all pass after the change (see commit/run notes).

## Update 2026-10-21 — the region rule now covers the Mega Hack-facing categories too

The previous update applied the code-region rule to `mod-identifier` and `other` only.
That left the five Mega Hack-facing categories holding rows that are provably
library-internal: a string referenced **only** by functions at/above `0x1803A4EE0`
cannot be a Mega Hack label, setting key, path, language key or format string, because
no Mega Hack code ever names it.

`scripts/classify_strings.py` now treats the region rule as a **promotion rule for every
category except the Mega Hack-facing ones**. The protected set is explicit:

```
PROTECTED_CATEGORIES = (
    "user-label", "format-string", "setting-key",
    "file-path", "language-key", "ui-element",
    "flag", "url", "api-path",
)
```

A row in any other category whose referencing functions are **all** in the library
region is reclassified `third-party`.

Effect on the inventory — **415 rows moved**, and not one Mega Hack-looking name among
them:

| From | To | Rows |
| ---- | -- | ---: |
| `user-label` | `third-party` | 230 |
| `format-string` | `third-party` | 184 |
| `file-path` | `third-party` | 1 (`.dll`) |
| `third-party` | `third-party` | — |
| **total** | | **415** |

Samples of what moved, all library text: `ANY PRIVATE KEY`, `PUBLIC KEY`,
`ENCRYPTED PRIVATE KEY`, `PrivateKeyInfo` (OpenSSL); `keymgmt key type = %s but legacy
type = EVP_PKEY_NONE` (OpenSSL); `calling fopen(%s, %s)` (c-ares); `DEK-Info: %s,`
(OpenSSL).

Verification, two independent angles:

1. **Region check** — no row in any of the five Mega Hack-facing categories has all of
   its referencing functions in the library region. Count: **0 leaks** in
   `user-label`, `setting-key`, `file-path`, `language-key` and `format-string`.
2. **Vocabulary check** — no row in `user-label`, `setting-key`, `file-path` or
   `language-key` carries a library symbol stem (OpenSSL/zlib/libc++ prefixes) or a
   library diagnostic. `format-string` has 13 rows that *look* like diagnostics
   (`invalid colour {}`, `failed to hook {:#010x}, err {}`, `error ruleset for tag {}
   not exist`, …), but each one is referenced only by Mega Hack's own functions
   (`0x1800830D0`, `0x180090BC0`, `0x1800BDA10`, `0x180108A80`, `0x180108E70`,
   `0x1801F8610`, `0x180204A60`, `0x180204F60`, `0x1802130E0` — all below
   `0x1803A4EE0`). They are Mega Hack's own log messages, and they stay.
3. **No Mega Hack vocabulary was swept away** — no `CHECKBOX_*`, `COLOUR_*`,
   `OPTIONS_*`, `STATUS/*` or `V_*` name appears in `third-party`.

This is the "check what is in a bucket, not how big it is" rule applied to the buckets
the previous pass never opened: the five categories the M1 criterion actually names.

## Open questions

- (Resolved 2026-10-21.) The `mod-identifier` vs OpenSSL ambiguity is settled by code-region
  evidence, not by an external symbol table: 957 of 980 rows are referenced only by functions
  at/above `0x1803A4EE0` (the statically-linked library region) and moved to `third-party`;
  the 23 survivors are referenced by Mega Hack's own code. No OpenSSL symbol table was needed.
  UNVERIFIED: the exact OpenSSL version (and therefore an authoritative symbol list) — the
  region boundary is inferred from the function-address gap, which is evidence about layout,
  not about version.
- (Resolved 2026-10-20.) A same-function suffix test (SSO text as a suffix of a VA-backed
  string in the same function) matches only 7 of the 181 SSO rows, so shape-based flagging
  is not viable; the 181 rows remain in `other`. See the rejected suffix/substring analysis
  above. UNVERIFIED: the decoder precision needed to separate them, which requires
  disassembly rather than text shape.
- (Resolved 2026-10-20.) All 77 `ui-element` names are registered by the CRT static
  initializer `FUN_180062940` (entry 11 of the 281-entry `_initterm` table), so they are
  live controls in the shipped build, not dead strings. UNVERIFIED: which names correspond to
  controls visible in the default UI versus gated by settings, and their M3 feature mapping.
- Separately: `/api/mega_hack/...` and the licence strings are **out of scope** — see
  [not-investigated-licence-path.md](not-investigated-licence-path.md). They appear here only
  because they are strings in the binary, and nothing in this note maps their behaviour.