# The `nxUI` menu layer

- **Question** — how do Mega Hack's menu rows, numeric tags and language definitions
  interact, and why does a packed interface leave no spare row for a new action?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Ground truth** — `absolllute.hackmega_original.dll`, SHA-256
  `1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92`.

## Evidence — two tabs, one registration site

- `hackpro::extensions::registerTab` (export `0x180072E50`) is a thin forwarder to
  `0x1800A60B0`; it is called exactly twice, both in `FUN_180002320`
  (`0x180002350` for `HOME`, `0x180002372` for `KEYBINDS`).
  See [tab-registration.md](tab-registration.md).
- Each tab is built from a gzip-compressed JSON stream (`FUN_1800BED70` /
  `FUN_1800BEE20`) and a distinct delegate (`0x180039360` = `core::HomeWin`,
  `0x18003D220` = `core::Keybinds`).

## Evidence — rows are addressed two ways

From [definition-keys.md](definition-keys.md):

- **Numeric row ids** (e.g. `0x1bd`, `0x1be`, `0x1bf`, `0x20`, `399`) are passed
  straight to the row builder `FUN_180120A40`. They are not strings.
- **String definition keys** (`HOME/...`, `ICONIC/...`, `0060`, `0095`) go through
  the exported `hackpro::define` (`0x1800A3C40`), which validates the key and interns
  it in `DAT_180801308` behind a thread-safe static initialiser.

`FUN_18011FFE0` is a complete worked example: it builds an `ICONIC` tab by repeatedly
calling `hackpro::define` for a prefix, opening a section, adding numeric rows, then
adding keyed rows.

## Evidence — tags, settings and rows are joined at runtime

- Settings are string-keyed and typed, interned into 64-bit `hackpro::Tag`s by
  `tagFor` (`0x180072DF0`) / `internKey` (`0x18007C700`).
  See [settings-registry.md](settings-registry.md).
- UI rows are not settings. A row id is a layout handle; its visible text comes from
  the definition manager, and its value (if any) comes from the settings registry via
  the tag resolved for that row's key.
- `FUN_18007CB60` (`tagFor` name resolver) and `FUN_18007C700` (`internTag`) are the
  two map operations that close the loop between a widget name and its `Tag`.

## Evidence — language dictionary load is a hook, not a rebuild of the tab

- The Home tab builder at `0x1800B6770` registers an action listener under the key
  `HOME/MISC/LOAD_LANG` (`0x1800B6770`, ~line 500 of the decompiled function) bound to
  the lambda whose body begins at `0x1800B88D0`.
- That lambda runs `FUN_180074D10` (`getConfigVar<std::string>`) and then
  `hackpro::extensions::addDefinition` (`0x1800754C0`), so a language switch re-interns
  definitions rather than rebuilding the (already registered) tab.
- `addDefinition` (`0x1800754C0`) writes into the definition manager at
  `DAT_180801308`; `hackpro::define` (`0x1800A3C40`) is the read side.

## Evidence — two-pass row rendering

- `FUN_1801697B0` toggles row `0095` off, draws, toggles it on, draws again. That is
  shadow/outline followed by the main pass, so a row can carry two render states without
  a second row slot.

## The "packed interface has no spare row" claim

Per `samples/absolllute.hackmega/RE_NOTES_PERSISTENT_BEST_RUN.md` (patch-side
engineering note, not independently re-derived from `_original.dll`):

- Mega Hack's tab row storage is packed, so there is no spare slot for an extra row.
- The Death Tracker **Import .dt** action therefore reuses the former **Show Prefix**
  row.
- Its tag is resolved only after core load; the boolean setter intercepts that exact tag
  and leaves every other setting untouched (implemented in the reconstruction as
  `Config::setHackEnabled` action-tag interception).
- A definition-insertion hook (the `HOME/MISC/LOAD_LANG` listener above) substitutes
  the label while each language dictionary is loaded, so the reused row stays visible in
  every language.

UNVERIFIED: the physical "no spare row" property of the row container. The binary
evidence above shows row reuse and label substitution; the claim that the container is
packed with no spare slot is taken from the patch engineering note and has not been
re-derived from a struct layout in `_original.dll`. Confirming would need the nxUI row
container layout (runtime-populated, not statically recoverable from this image).

## Conclusion

1. Rows are layout handles (numeric ids) plus text (definition keys); text is resolved
   through the interned definition manager, values through the tagged settings registry.
2. Language changes do not rebuild tabs; they re-run the `LOAD_LANG` action listener,
   which calls `addDefinition` for the new dictionary.
3. The Import .dt reuse of the Show Prefix row is consistent with this model: the row
   slot already exists, only its label and its tag's handling change.
4. The packed-container explanation is plausible and matches the observed reuse, but
   remains UNVERIFIED as a struct-layout claim.

## Open questions

- UNVERIFIED: the nxUI row container struct and whether it truly has no spare slot.
- UNVERIFIED: the exact method on `core::HomeWin` / `core::Keybinds` that renders a row
  from its numeric id (vtables are runtime-populated).
- UNVERIFIED: whether any third-party tab can be registered at runtime (no static call
  sites beyond the two above).
