# The definition-key and row-id system

How Mega Hack addresses labels. This is the subsystem any text change has to go through.

- **Question** — how does Mega Hack name and look up its strings, and where would a
  session-vs-global label change have to live?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Method

1. Scan `.rdata` for NUL-terminated strings matching a definition-key shape — either a
   numeric id (`0052`) or a slash-separated upper-case path (`ICONIC/EFFECTS`).
2. Disassemble all of `.text` with capstone and index every rip-relative reference to those
   addresses, then group keys by the 4 KiB code region that reads them.
3. Resolve each referencing function through `recovered/functions/` and read the
   decompiled Ghidra output.

## Evidence — the key inventory

176 definition-key-shaped strings in `.rdata`: **84 path-shaped, 92 numeric**.

`STATUS` exists as a standalone string at file offset `0x5339F0` — the first byte of
`.rdata`. It is a namespace root, not a key.

Only **62 of the 176** keys have any code reference; the rest are inert, which is consistent
with keys present in a language file but unused by this build. Inventory written to
`recovered/definition_keys.tsv`.

Path keys are grouped by top-level namespace, which is the feature taxonomy:

| Namespace | Count | Examples |
| --------- | ----: | -------- |
| `HOME/` | 20 | `HOME/LANGUAGE`, `HOME/ACCENT`, `HOME/AUTO_UPDATE`, `HOME/TAB_TEXT` |
| `ICONIC/` | 22 | `ICONIC/PREVIEW/TRAIL`, `ICONIC/SPECIAL/SPEED/PRE`, `ICONIC/VANILLA_PROGRESS_BAR` |
| `ERROR/` | 11 | `ERROR/LEVEL/NO_CBF`, `ERROR/HACKPRO/CONFLICTING_RULESET`, `ERROR/JSON_PARSE_FAIL` |
| `MESSAGE/` | 13 | `MESSAGE/CHEAT_SAFETY/CONFIRM_DISABLE`, `MESSAGE/UTILITY/CRASH/0..2` |
| `KEYBINDS/` | 5 | `KEYBINDS/SHORTCUT`, `KEYBINDS/VERSION`, `KEYBINDS/DISABLE_IN_EDITOR` |
| `NX/` | 7 | `NX/COLOUR/{R,G,B,H,S,V}` |
| `DISPLAY/`, `W5/`, `S/`, `A/` | 1 each | `DISPLAY/HACK_BORDERLESS_CLASSIC`, `W5/6`, `S/MIME`, `A/P` |

Numeric keys are **row and label ids**, not text. They appear in pairs and triples at a
single address range (`0x180571640`: `0060`, `0061`; `0x18057164a`: `0096`) and are consumed
as integers.

## Evidence — how a row is built

`recovered/functions/18011ffe0_FUN_18011ffe0.c` constructs a whole settings tab. Ghidra shows
the string literals as MSVC SSO stack buffers; decoding the immediates gives them back:

| Decoded from | String |
| ------------ | ------ |
| `0x312e3976`, `0x332e` | `v7.13` |
| `0x4e4f4349`, `0x4349`, `0x2f`, `0x41534944`, `0x454c42` | `ICONIC/DISABLE` |
| literal `builtin_strncpy` | `ICONIC/VANILLA_PROGRESS_BAR` |
| `0x4e4f4349`, `0x4349`, `0x2f` | `ICONIC/` (used as a prefix) |
| `0x454d4f48`, `0x442f`, `0x4f`, `0x54` | `HOME/DOT` |
| `0x4e4f4349`, `0x4349`, `0x2f`, `0x444e4152`, `0x454c42` | `ICONIC/RAND` |
| `0x4e4f4349`, `0x4349`, `0x2f`, `0x454e4547`, `0x4c4152` | `ICONIC/GENERAL` |
| `0x4e4f4349`, `0x4349`, `0x2f`, `0x45464645`, `0x535443` | `ICONIC/EFFECTS` |
| `0x4e4f4349`, `0x4349`, `0x2f`, `0x45564157` | `ICONIC/WAVE` |

The pattern is uniform and repeats six times:

```
build std::string prefix from SSO immediates
pbVar6 = hackpro::define(&prefix);              // exported, 0x1800A3C40
tab    = FUN_1801ece60(pbVar6);                 // open a section by name
FUN_180120a40(param_1, tab, <numeric row id>);  // add rows
FUN_180120ca0(param_1, tab, &key);              // add a keyed row
FUN_1801ebdb0(...,"GJ_button_01.png","GJ_button_02.png",tab);
```

So:

- `hackpro::define(const std::string&) -> const std::string&` (exported, VA `0x1800A3C40`)
  resolves a key to its text. It returns a reference, so it is an interning lookup into a
  table populated from the language file.
- Numeric ids (`0x1bd`, `0x1be`, `0x1bf`, `0x20`, `0x1b1`, `0x1ba`, `0x1a6`, `0x1b4`, `399`,
  `0x1b0`, `0x192`, `0x18e`, `0x1b2`, `0x18b`, `0x1c0`, `0x19b`, `0x18a`, `0x18d`, `0x19e`,
  `0x18c`) are row identifiers passed straight to the row builder. They are not strings at
  all — they are enum-like constants.
- String keys (`ICONIC/GENERAL`, `HOME/DOT`, and the four-digit ones like `0060`) go through
  `hackpro::define` and are used as section or row names.

**This is the mechanism behind `tab-registration.md`:** the two tabs registered at
`0x180002350` and `0x180002372` are the two definition tables these row builders populate.
The `std::istream` in `registerTab` carries the tab's row description.

## Evidence — row 0095 is drawn twice

`recovered/functions/1801697b0_FUN_1801697b0.c`, the only other function in the
`showNewBest` neighbourhood that touches a definition key:

```c
void FUN_1801697b0(undefined8 param_1)
{
  undefined8 uVar2;
  char cVar1;

  uVar2 = FUN_180147ba0();
  cVar1 = FUN_180148610(uVar2,&DAT_1805759a7);      // key "0095" present?
  if (cVar1 != '\0') {
    FUN_180149670(uVar2,&DAT_1805759a7,0);          // row 0095 off
    FUN_180110790(param_1);                          // draw
    FUN_180149670(uVar2,&DAT_1805759a7,1);          // row 0095 on
    return;
  }
  FUN_180110790(param_1);                          // draw
  return;
}
```

Setting a row off, drawing, setting it on, drawing again is the standard two-pass text
render: shadow/outline pass first, main pass second. So row `0095` is a text row with two
rendering states, and `FUN_180149670(registry, key, value)` is its setter.

Recalling [new-best-label.md](new-best-label.md), row `0052` lives in the same 4 KiB region
and is read only by the two `showNewBest` replacements, where key-absent selects opacity
`1.0` and key-present `0.5`. So `0052` and `0095` are two distinct rows of the same tab,
both involved in the New Best indicator.

## The negative result

**There is no session/global label pair anywhere in the binary.** Searching all printable
ASCII runs, the only `Global` strings are `GlobalAlloc`, `GlobalFree`, `GlobalLock`,
`GlobalUnlock` (KERNEL32 imports) and `Global default library context` (a .NET string, not
Mega Hack's). There is no `Session` string either. The only `Best` strings are
`Best Run: {}` and `PlayLayer::showNewBest`.

So the session-vs-global distinction is **not a text swap between two existing labels**.
Nothing in the release names the two modes. Three possibilities remain, in order of
likelihood:

1. **It is a value comparison, not a label.** The indicator may show "new best" when the run
   beats a *session* value, and the change the user wants is to compare against the stored
   all-time value instead. Nothing new needs naming.
2. **GD supplies the text.** `PlayLayer::showNewBest` is RobTop's method; Mega Hack only
   controls opacity (verified in new-best-label.md). If the words "new best" appear in game,
   they come from Geometry Dash, not from Mega Hack, and Mega Hack cannot change them
   without replacing the label node.
3. **A new key would have to be added.** The language-file format would need a new key, and
   Mega Hack's shipped dictionaries would not have it, so the label would render as the
   missing-translation case — which, per `0052`, is exactly the 0.5-opacity path.

Reading 1 is most likely and is also the cheapest to test. UNVERIFIED: which of the three it
is. The discriminator is `FUN_180110790` and `FUN_180149670` — decompiling them should show
which value is compared against which.

## Conclusion

1. Labels are addressed by two mechanisms: named string keys resolved through the exported
   `hackpro::define`, and numeric row ids passed directly to row builders.
2. The two tabs registered at load are the definition tables for those rows, which closes
   the loop with `tab-registration.md`.
3. `ICONIC/` is the feature namespace with 22 keys, and `FUN_18011FFE0` is a complete worked
   example of building one tab.
4. Row `0095` uses two-pass rendering (shadow then main); row `0052` selects label opacity
   1.0 or 0.5 depending on dictionary presence.
5. No session/global label pair exists in the binary, so that change is a value-comparison
   change or needs a new key — not a text substitution.

## Open questions

- Decompile `FUN_180110790` (the draw call shared by both New Best paths) and `FUN_180149670`
  (the row setter). These will show what value the comparison uses, which decides between
  possibilities 1 and 2 above.
- Enumerate every `FUN_180120A40` / `FUN_180120CA0` call site to build the full row-id table
  and its owning tab. That table is the map any UI change needs.
- (Partially resolved 2026-10-20.) 245 (`0xF5`) gates whether `FUN_18017F000` uses a
  1.0 or 0.5 delay (via `FUN_180148610`); 247 (`0xF7`) enables the delay scaling inside
  `FUN_18017F000`; 501 (`0x1F5`) and 502 (`0x1F6`) are read in `FUN_18017F000` and in
  the cheat helpers `areCheatsEnabled`/`disableAllCheats`. UNVERIFIED: the human-readable
  setting names (hackpro.json keys) for all four.
- Establish how `hackpro::define` misses are handled, and whether a missing key yields empty
  text or the key itself.
- (Resolved 2026-10-20.) The second tab registered in `FUN_180002320` uses the
  `core::Keybinds` delegate (`FUN_18003D220`), while the first uses `core::HomeWin`
  (`FUN_180039360`), so the second tab is the Keybinds tab. `FUN_18011FFE0` (ICONIC) is one
  of the two tab bodies fed by the compressed blobs. UNVERIFIED: which blob belongs to which
  tab.

## Note on tooling

Everything above came from `recovered/functions/`. The grep-to-function workflow — index the
keys, find the referencing function, read its decompiled source — is fast enough to be the
default method for UI and localisation questions, and far cheaper than Ghidra navigation by
hand.
