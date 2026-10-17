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
| `other` | 1,359 | see "what is still missing" below |
| `mod-identifier` | 974 | Mega Hack's own snake_case names — `icons_0`, `override_cols`, `player_ball`, `no_bar`, `same_dual_col` |
| `user-label` | 479 | UI text |
| `format-string` | 283 | printf and `{}` templates |
| `third-party` | 272 | statically linked OpenSSL/zlib, gated on a known prefix |
| `host-method` | 214 | `PlayLayer::createCheckpoint`, `cocos2d::CCDirector::popScene` |
| `flag` | 153 | `HACK_COPY_HACK+` and similar |
| `setting-key` | 126 | `ICONIC/DISABLE`, `HOME/THEME/{}/COL` |
| `mangled-symbol` | 96 | MSVC `?f@ns@@` and `??1Class` forms |
| `file-path` | 86 | asset and data filenames |
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

## What is still missing

`other` is 1,359 rows — **33% of the inventory**, down from 76% at the first pass but still
the largest single group. Sampling shows it is genuinely heterogeneous rather than one missed
rule: short fragments (`init`, `object`, `zFI`), remaining OpenSSL and zlib names
(`PEM_do_header`, `png_set_sPLT`, `group-check`), embedded punctuation, and some decoding
noise.

M1's criterion asks for the inventory to be **split**, and a split that leaves a third of it
in one bucket does not meet it. The criterion therefore stays unchecked. The number is left
visible rather than tuned away.

## Conclusion

1. 4,145 strings classified into 16 categories, 3,609 VA-anchored and 536 SSO-only.
2. Mega Hack's own widget names are recoverable, which is a direct M3 lead.
3. `other` is still 33%, so the criterion is not yet met.
4. The largest recovered group after `other` is Mega Hack's own 974 identifiers, not
   third-party code — which corrects the impression that this binary is mostly OpenSSL.

## Open questions

- Resolve `mod-identifier` against the OpenSSL symbol table to split Mega Hack's names from
  the library's.
- Work down `other`, starting with the OpenSSL/zlib remainder.
- Confirm whether every `ui-element` name maps to a live control, and extend that group into
  the M3 feature map.
- Separately: `/api/mega_hack/...` and the licence strings are **out of scope** — see
  [not-investigated-licence-path.md](not-investigated-licence-path.md). They appear here only
  because they are strings in the binary, and nothing in this note maps their behaviour.