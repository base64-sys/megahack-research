# RTTI and the class hierarchy

- **Question** — M1 requires Mega Hack's own RTTI/vtable set enumerated and the class
  hierarchy recovered. What classes exist, and where are their vtables?
- **Baseline** — Mega Hack v9.1.3, GD 2.2081, Geode 5.6.1.
- **Date** — 2026-10-17.
- **Tool** — `scripts/rtti_index.py` → `recovered/functions/_rtti.tsv`.

## Method

MSVC x64 RTTI is three linked structures, so it is walked rather than guessed:

| Structure | Layout |
| --------- | ------ |
| `TypeDescriptor` | `pVFTable(8) spare(8) name(".?AVFoo@@\0")` |
| `CompleteObjectLocator` | `signature(4) offset(4) cdOffset(4) pTypeDescriptor(4) pClassHierarchy(4) pSelf(4)` |
| vtable | `vtable[-1]` points at the COL; `vtable[0]` is the first virtual |

Three passes: find TypeDescriptors by their `.?A` name, find COLs by
`signature == 1` **and** `pSelf == own RVA`, then find vtables by locating the
pointer to a COL — the vtable starts 8 bytes after it.

| Stage | Count |
| ----- | ----: |
| Type descriptors | 517 |
| Complete object locators | 497 |
| Vtables resolved | **497** |

The `pSelf == self` test is what makes this trustworthy rather than a coincidence
count. A 24-byte window starting with the right signature occurs by chance in
8.7 MB of `.rdata`; one that also points at itself does not. All 497 pass both.

## Failure mode worth recording

The first implementation indexed the file buffer with an **RVA** — `unpack(data, rva)` —
which is precisely the mistake `AGENTS.md` and `notes/provenance.md` document. It did not
crash. It reported "517 type descriptors, 0 COLs", which reads like a plausible finding.

The tell was that `pe_rva.py` had no `.base` attribute, so `image.base` raised
`AttributeError` in a later line; following that up by hand showed the buffer offsets and
the RVAs were in completely different places.

`pe_rva.py` now has `Image.rva_of_offset()` as the documented inverse of
`va_of_file_offset()`, so the conversion has one implementation instead of three inline
attempts. All scans here walk **file offsets** and convert outward.

This is the third time this repo's own RVA rule has been violated by code written against
it, which is worth more than any single fix: the rule needs to be a function, not a habit.

## Mega Hack's own classes

Excluding `std::`, `geode::` and lambda closures, the recovered hierarchy:

| Namespace | Classes |
| --------- | ------- |
| `hackpro::` | `Config`, `BasicConfig` |
| `hackpro::ui::` | `Object`, `Option`, `Checkbox`, `Spinner`, `Shortcut`, `Colour`, `TextBox`, `Picker` |
| `core::` | `HomeWin`, `Keybinds` |
| `mega::extensions::` | `Home` |
| `mega::utils::` | `Resources`, `CompressedBuffer`, `EncryptedBuffer` |
| `tabcore::` | `menu::Menu`, `text::Label` |

`hackpro::ui::*` at vtables `0x180543698`-`0x180543708` are consecutive 16-byte-strided
entries, which is what a single base class with derived types produces — consistent with
`ui::Object` being the root and the rest deriving from it.

## Two things this adds that nothing else had

**A third namespace: `tabcore`.** `tabcore::menu::Menu` and `tabcore::text::Label` appear at
seven and two vtables respectively. Neither `hackpro`, `hackpro::extensions` nor
`mega::utils` — the namespaces previously known from the export table — covers it. M3 will
need this.

**`mega::utils::EncryptedBuffer` and `CompressedBuffer`.** Buffers with an inner
`InputBuffer`, at vtables `0x180547A98`/`0x180547C98`. Follow-up established that the
encrypted buffer sits in the **licence/entitlement validation path** — OpenSSL statically
linked, a login endpoint, `Error validating license`, `Serial Number:` — which
`AGENTS.md` puts out of scope. Investigation stopped there on purpose; see
[not-investigated-licence-path.md](not-investigated-licence-path.md). `CompressedBuffer` is
compression rather than licensing and remains in scope.

**Bundled third-party code.** `nlohmann::json_abi_v3_12_0::detail::{exception, type_error,
out_of_range, invalid_iterator, parse_error}` — Mega Hack statically links nlohmann/json
**3.12.0**. This matters for the save/load format work in M1: JSON parsing is in-binary,
not a Geode or host facility.

`geode::UnwrapException` at `0x180541998` confirms Geode's exception wrapper is linked in.

## Limits

`demangle()` extracts the class name and its namespaces and does nothing else. It is not a
demangler, and for lambda closures and templates it produces nonsense — `std::XPEAVNXObject::Z::PEBAVNXDirector::...`
is what a closure comes out as. The exact mangled name is kept in the `mangled` column for
every row, so nothing is lost. Real demangling stays with `undname.exe`, as used for the 30
exports in [hackpro-api.md](hackpro-api.md).

## Inheritance: solved

`pClassHierarchy` decodes, and **the layout is not the documented one.** MSVC's
`_RTTIBaseClassHierarchyDescriptor` is `pNumBaseClasses(4) pBaseClassArray(4)`, but in this
binary `numBases` sits at **+8**, with the array RVA at +12 and two zero dwords ahead of it.
Read at +0 it yields `numBases == 0` for every class, which claims Mega Hack has no
inheritance at all — refuted immediately by the vtable layout.

```
pClassHierarchy + 0    0x00000000   unused
pClassHierarchy + 4    0x00000000   unused
pClassHierarchy + 8    numBaseClasses
pClassHierarchy + 12   RVA of the array
pClassHierarchy + 16   first array entry, inline
```

Each array entry is an RVA to a Base Class Descriptor: `pTypeDescriptor(4)
numContainedBases(4) mdisp(4) pdisp(4) vdisp(4)`. Entry 0 is the class itself; later entries
are its bases most-derived first, with `numContainedBases == 0` marking a direct base.

Confirmed against `hackpro::ui::*`, whose inheritance was already known from the 16-byte
vtable stride. All seven derive **directly** from `hackpro::ui::Object`, which is itself the
root — so the earlier guess that `Option` parented `Checkbox` was wrong, and testing it is
what caught that.

| Metric | Count |
| ------ | ----: |
| Raw edges | 3,102 |
| Distinct (derived, base) pairs | **757** |
| Classes with at least one base | 279 |

Raw exceeds distinct because a class with *N* vtables has *N* COLs contributing the same base
list; the index de-duplicates.

### Mega Hack's own hierarchy

In-project edges only, excluding cocos2d and GD host types:

| Derived | Base |
| ------- | ---- |
| `hackpro::ui::{Option,Checkbox,Spinner,Shortcut,Colour,TextBox,Picker}` | `hackpro::ui::Object` |
| `hackpro::Config` | `hackpro::BasicConfig`, `hackpro::NodeDelegate` |
| `core::HomeWin` | `mega::extensions::Home`, `hackpro::extensions::Delegate` |
| `core::Keybinds` | `hackpro::extensions::Delegate` |
| `mega::extensions::Home` | `hackpro::extensions::Delegate` |
| `mega::utils::Resources` | `hackpro::DataDelegate` |
| `iconic::SpecialPastel` | `iconic::SpecialRGB` → `iconic::SpecialCore` |
| `iconic::{SpecialRGB,SpecialGradient,SpecialCustom}` | `iconic::SpecialCore` |
| `tabcore::menu::{ItemToggle,Toggle,Input,Button}` | `tabcore::menu::Item` |
| `tabcore::menu::Spinner` | `tabcore::menu::Input` → `tabcore::menu::Item` |
| `tabcore::layout::{Grid,SwitcherContainer}` | `tabcore::layout::ScrollerDelegate` |
| `tabcore::dialog::SwitcherNode` | `tabcore::menu::Menu` |

Three things worth stating plainly:

1. **`hackpro::ui::Object` is a flat root.** Seven widget types, all direct children, none
   derived from each other. That is why the vtables are 16 bytes apart.
2. **`iconic::` is a three-level hierarchy** — `SpecialCore` → `SpecialRGB` → `SpecialPastel`
   — which is the only genuine multi-level chain among Mega Hack's own classes.
3. **Multiple inheritance is real.** `hackpro::Config` derives from both `BasicConfig` and
   `NodeDelegate`, and `core::HomeWin` from both `mega::extensions::Home` and
   `hackpro::extensions::Delegate`. This is the same shape as the `hackpro::Config` COL pair
   seen earlier, where the second COL carries `offset = 0x128`.

`session::Login`, `core::NoclipTracker` and `tabcore::dialog::ColourPicker` have base names
that are clearly mis-demangled — `tabcore::std`, and a nonsense `tabcore::...` string — so
their bases are recorded but not trusted. The `mangled` column in `_rtti.tsv` is authoritative.

## Conclusion

1. 497 polymorphic classes with vtables recovered from `_original.dll`, each anchored to a VA.
2. 757 distinct inheritance edges — `numBases` lives at `+8`, not `+0` as documented.
3. Mega Hack's namespaces are `hackpro`, `core`, `mega::extensions`, `mega::utils`, `tabcore`,
   and `iconic`.
4. nlohmann/json 3.12.0 is statically linked.

## Open questions

- Three classes have mis-demangled base names; `undname.exe` on their mangled names would
  resolve them.
- `mega::utils::CompressedBuffer` is unexamined and in scope; the encrypted sibling is not.
- Check whether the Android `.so` carries the same class set, which is the M5 question.