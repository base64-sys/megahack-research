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
`InputBuffer`, at vtables `0x180547A98`/`0x180547C98`. Given the persistence work, an
encrypted buffer type in `mega::utils` is directly relevant and was not previously on the map.

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

UNVERIFIED: inheritance relationships between the COLs. `pClassHierarchy` is captured but
not yet decoded — a Base Class Descriptor is a chain of 32-bit RVAs terminated by zero, and
walking it is the obvious next step toward a true hierarchy rather than a flat list.

## Conclusion

1. 497 polymorphic classes with vtables recovered from `_original.dll`, each anchored to a VA.
2. Mega Hack's namespaces are `hackpro`, `core`, `mega::extensions`, `mega::utils` and —
   newly — `tabcore`.
3. nlohmann/json 3.12.0 is statically linked.
4. Inheritance edges are captured but not decoded.

## Open questions

- Decode `pClassHierarchy` to recover the actual base-class graph.
- `mega::utils::EncryptedBuffer` is unexamined and relevant to the persistence work.
- Check whether the Android `.so` carries the same class set, which is the M5 question.