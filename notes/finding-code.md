# Finding code by name, and the Best Run formatter

The practical bottleneck for changing anything, and what the index turned up on the way.

- **Question** — given a feature name, how do you find the code in an 18,800-file tree?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/build_xref_index.py` → `recovered/functions/_xrefs.tsv`.

## Method

`tools/pe_xrefs.py` answers this for a handful of strings at a time, by disassembling. For
the whole image at once, two passes over the recovered Ghidra output are cheaper and need
no disassembler:

1. **Symbols.** Every rip-relative data reference Ghidra could not resolve to a name appears
   as `DAT_<hex>`. Those map to a VA directly.
2. **Literals.** A string Ghidra *did* decode is inlined as a `"..."` literal and never gets
   a `DAT_` symbol. Those are matched back by content.

Pass 1 alone found only **159** referenced strings. Adding pass 2 took it to **3,611** — a
22× improvement, and the difference between the index being useless and being useful.

String discovery itself uses `scripts/pe_rva.py`, so zero-fill and gap addresses are
refused rather than read out of a neighbouring section.

Hook-wrapper thunks are folded in: a reference made from a thunk is credited to the function
actually hooked, using `_wrappers.tsv` from [hook-wrapper-layer.md](hook-wrapper-layer.md).

## Evidence — the index

```
strings found        : 107538
strings referenced    : 3611
  DAT_ references     : 7555
  literal references  : 8480
recovered functions   : 18825
hook targets folded in: 267
```

`_xrefs.tsv` is one row per referenced string: VA, ref count, referencing functions (thunks
already rewritten to hook targets), and the text. `--grep` queries it directly:

```powershell
python scripts/build_xref_index.py --grep best
```

```
0x1805759e9  [1 refs]  'PlayLayer::showNewBest'
      180169de0_FUN_180169de0
0x1805b8d8a  [1 refs]  'Best Run: {}'
      18024e8d0_FUN_18024e8d0
```

The first entry independently reproduces the hook registrar found by hand in
[new-best-label.md](new-best-label.md) — a second angle on that claim, from a completely
different tool.

## New finding: the Best Run display formatter

`FUN_18024E8D0`, newly surfaced by the index, is the formatter behind the `Best Run: {}`
template. It parses format tokens and streams values into a `std::basic_ostream`.

Two tokens identified from the disassembly, decoded from little-endian immediates:

| Comparison | Token | Behaviour |
| ---------- | ----- | --------- |
| `*(int*)plVar1 == 0x64656564`-style, `case 4` | `dead` | emits `"Alive"`, or `"Dead"` when `*(char*)(this + 0x324A)` is set |
| `*(int*)plVar1 == 0x706D756A`, `case 5` | `jump` | gated on setting id `0x321` (801) |
| `*(short*)plVar1 == 0x7063` with `+2 == 's'` | `cps` | streams a rate, optionally appending `/` and two more terms when settings `0x2F9`, `0x2FA` (761, 762) are on |

The `cps` branch is worth noting: it composes the display from up to three values with `/`
separators, each conditional on its own setting. That is how Mega Hack shows a multi-part
rate like `x/y/z` and lets the player pick which parts appear.

So the Best Run display is a **template with named tokens**, not a fixed string. A change to
what it shows is a change to which tokens the template contains, not a string edit.

UNVERIFIED: the full token set. `FUN_18024E8D0` is 646 lines with a `switch` over token
lengths; other `case` arms were not read. Confirming needs the full body, not a sample.

## The session/global question, third confirmation

Querying `session` across all 3,611 referenced strings returns only:

| String | Function |
| ------ | -------- |
| `opening session` | `FUN_180407FE0` |
| `closing session` | `FUN_180407FE0` |
| `session_id` | `FUN_1804ACD00`, `FUN_1804ACFB0` |

Those are logging and OpenSSL. **No Mega Hack feature names a session.** This is now the
third independent pass to find one — the raw binary scan, the definition-key inventory, and
now the code-referenced string index. The conclusion in
[definition-keys.md](definition-keys.md#the-negative-result) holds and strengthens: the
session-vs-global change is a value comparison or needs a new key.

Best next probe: `FUN_18002DE50(setting, 0x321)` and the ids around it. If a setting controls
whether `jump` appears in the Best Run line, the settings registry near 801 is where a
session/global switch would live.

## Conclusion

1. `build_xref_index.py` makes the recovered tree searchable by feature name, which is the
   precondition for modifying anything in it.
2. Literal matching is essential; symbol matching alone finds 4% of the answers.
3. `Best Run: {}` is a tokenised template with `dead`, `jumps` and `cps` tokens identified.
   Mega Hack names no session anywhere.

## Failure modes recorded

- A first pass matching only `DAT_` symbols reported 159 referenced strings and looked like
  a broken tool. It was not broken; Ghidra names only the references it cannot decode. Always
  match inlined literals as well as symbols.
- Editing the script with PowerShell regex replacements broke indentation and produced an
  `IndentationError`. Use the editor for structural changes; regex is for the content only.