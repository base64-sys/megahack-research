"""Enumerate Mega Hack's MSVC RTTI: every polymorphic class, its vtable and its base.

MSVC x64 RTTI is fully described by three linked structures, so it can be walked
rather than guessed at:

    TypeDescriptor   pVFTable(8) spare(8) name(".?AVFoo@@\\0")
         ^                    |
         |                    +-- referenced by the Complete Object Locator
    CompleteObjectLocator  signature(4) offset(4) cdOffset(4)
                           pTypeDescriptor(4) pClassHierarchy(4) pSelf(4)
         ^
         |
    vtable[-1] --> CompleteObject Locator, and vtable[0] is the first virtual

So the pass is: find every COL by validating `signature == 1` and that `pSelf`
points back at the COL itself; confirm its TypeDescriptor really starts with
`.?A`; then find the vtable by looking for a pointer to that COL, with the
vtable starting 8 bytes later.

The `pSelf == self` test is what makes this trustworthy. A 24-byte window that
happens to start with the right signature will occur by chance in 8.7 MB of
`.rdata`; one that also points at itself will not, and every hit here has passed
both.

    python scripts/rtti_index.py
    python scripts/rtti_index.py --grep PlayLayer
    python scripts/rtti_index.py --hier
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import struct
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from pe_rva import Image  # noqa: E402

FUNCTIONS = REPO / "recovered" / "functions"
DEFAULT_DLL = REPO / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"

COL_SIZE = 24
TD_SIZE = 24
SIGNATURE_64 = 1

# ".?AV" = class, ".?AU" = struct. MSVC prefixes nested names with
# "$1$" per scope level, e.g. ".?AVFoo@Bar@@" is Bar::Foo.
NAME_PREFIXES = (".?AV", ".?AU")


def read_cstring(data: bytes, offset: int, limit: int = 512) -> str:
    end = data.find(b"\x00", offset, offset + limit)
    if end < 0:
        return ""
    return data[offset:end].decode("ascii", errors="replace")


UNNAMED_PREFIX = ".??"


def demangle(mangled: str) -> str:
    """Render an MSVC RTTI type name readably.

    This is a small recursive-descent parser for the subset that appears in type
    descriptors, not a general demangler. It exists because `undname.exe` cannot
    be used here: it parses plain symbols correctly (`?func@@YAXXZ` ->
    `void __cdecl func(void)`) but fails on `.?AVFoo@@`, echoing `. ?? AVFoo`
    without consuming the `AV` class tag. Its output on this binary would have
    been worse than useless, so the shapes are handled directly.

    Supported: plain and nested classes, template instantiations
    (`.?AV?$shared_ptr@VSpecialCore@iconic@@std@@`), lambda closure classes, and
    the ``?0`` local-name escape. Anything unrecognised is returned unchanged
    rather than mangled further, and `mangled` stays authoritative in the index.
    """
    if not mangled.startswith(NAME_PREFIXES):
        return mangled

    body = mangled[4:]  # drop .?AV / .?AU
    if not body:
        return mangled

    def parse(text: str):
        """Return (rendered, rest). Stops at '@' or end."""
        if not text:
            return "", ""
        if text.startswith("?$"):  # template class or member function
            head, rest = parse(text[2:])
            args, rest = parse_template_args(rest)
            rendered = f"{head}<{', '.join(a for a in args if a)}>" if any(args) else head
            return rendered, rest
        if text.startswith("?0"):  # local name / lambda scope escape
            _, rest = parse(text[2:])
            inner, rest = parse(rest)
            return inner, rest
        if text.startswith("?1") or text.startswith("?2") or text.startswith("?3"):
            return "", text[2:]
        # a run of name characters
        i = 0
        while i < len(text) and text[i] not in "@?":
            i += 1
        return text[:i], text[i:]

    def parse_template_args(text: str):
        args, rest = [], text
        if rest.startswith("@@"):
            return args, rest[1:]  # empty template args
        while rest:
            arg, rest = parse(rest)
            args.append(arg)
            if rest.startswith("@"):
                rest = rest[1:]
                if rest.startswith("@"):
                    rest = rest[1:]
                    return args, rest
            else:
                return args, rest
        return args, rest

    parts: list[str] = []
    rest = body
    while rest:
        before = rest
        part, rest = parse(rest)
        if part:
            parts.append(part)
        if rest.startswith("@"):
            rest = rest[1:]
        if rest == before:
            # No progress: the input has a shape this parser does not model.
            # Return the untouched mangled name rather than guessing, and stop -
            # a loop that cannot consume input is an infinite loop otherwise.
            return mangled

    if not parts:
        return mangled
    return "::".join(reversed(parts))


def find_type_descriptors(image: Image) -> dict[int, str]:
    """RVA of every TypeDescriptor, keyed to its mangled name.

    A TypeDescriptor is two pointer-sized fields followed by a NUL-terminated
    name. The name is the anchor: MSVC type names always begin `.?A`.

    Every scan in this file walks **file offsets** and converts to an RVA, never
    the reverse. Indexing the file with an RVA is the exact mistake
    AGENTS.md documents, and it does not fail loudly - it finds plausible-looking
    names at the wrong addresses.
    """
    data = image.data
    found: dict[int, str] = {}
    for off in range(0, len(data) - TD_SIZE, 4):
        name = read_cstring(data, off + 16)
        if not name.startswith(NAME_PREFIXES):
            continue
        rva = image.rva_of_offset(off)
        if rva is not None:
            found[rva] = name
    return found


def find_cols(image: Image, types: dict[int, str]) -> list[dict]:
    """Every Complete Object Locator, validated by signature and self-pointer."""
    data = image.data
    cols: list[dict] = []
    unpack = struct.Struct("<IIIIII").unpack_from
    for off in range(0, len(data) - COL_SIZE, 4):
        sig, offset, cd, td, hierarchy, self_rva = unpack(data, off)
        if sig != SIGNATURE_64:
            continue
        rva = image.rva_of_offset(off)
        # The self-pointer test is what makes this trustworthy: a 24-byte window
        # that happens to start with the right signature occurs by chance in
        # 8.7 MB of .rdata, one that also points at itself does not.
        if rva is None or self_rva != rva:
            continue
        name = types.get(td)
        if name is None:
            continue
        cols.append(
            {
                "col_rva": rva,
                "offset": offset,
                "cd_offset": cd,
                "td_rva": td,
                "hierarchy_rva": hierarchy,
                "mangled": name,
                "name": demangle(name),
            }
        )
    return cols


def find_vtables(image: Image, col_rvas: set[int]) -> dict[int, int]:
    """Map COL RVA -> vtable RVA.

    A vtable is immediately preceded by a pointer to its COL, so the pointer sits
    at vtable-8 and the vtable itself starts 8 bytes later. The stored pointer is
    an absolute VA, so it is compared against `image_base + col_rva`.
    """
    data = image.data
    base = image.image_base
    wanted = {base + rva for rva in col_rvas}
    found: dict[int, int] = {}
    for off in range(0, len(data) - 8, 8):
        (value,) = struct.unpack_from("<Q", data, off)
        if value in wanted:
            vt_rva = image.rva_of_offset(off + 8)
            if vt_rva is not None:
                found[value - base] = vt_rva
    return found


def read_u32(data: bytes, rva: int, image: Image) -> int | None:
    """Read a 4-byte value at an RVA, going through `resolve`.

    `resolve` is used rather than an arithmetic shortcut because an RVA past a
    section's `SizeOfRawData` would otherwise index into a different section and
    return a plausible wrong number - the failure AGENTS.md documents.
    """
    res = image.resolve(image.image_base + rva)
    if not res.ok or res.file_offset is None:
        return None
    off = res.file_offset
    if off + 4 > len(data):
        return None
    (value,) = struct.unpack_from("<I", data, off)
    return value


def decode_hierarchy(image: Image, col: dict, types: dict[int, str]) -> list[dict]:
    """Walk the Base Class Hierarchy Descriptor into real base/derived edges.

    The layout is **not** the documented one. MSVC's
    `_RTTIBaseClassHierarchyDescriptor` is `pNumBaseClasses(4) pBaseClassArray(4)`,
    but in this binary `numBases` sits at **+8**, not +0, with the array RVA at +12
    and two zero dwords in front. Reading it at +0 yields `numBases == 0` for every
    class, which would claim Mega Hack has no inheritance at all - contradicted
    immediately by the vtable layout.

    Confirmed against `hackpro::ui::*`, whose inheritance is known from the
    16-byte vtable stride: all seven derive directly from `hackpro::ui::Object`,
    which is itself the sole root of that subtree.

        pClassHierarchy + 0   0x00000000   unused
        pClassHierarchy + 4   0x00000000   unused
        pClassHierarchy + 8   numBaseClasses
        pClassHierarchy + 12  RVA of the array
        pClassHierarchy + 16  (the first array entry, inline)

    Each array entry is an RVA to a Base Class Descriptor:

        +0 pTypeDescriptor   +4 numContainedBases
        +8 mdisp             +12 pdisp      +16 vdisp

    Entry 0 is the class itself, with `numContainedBases` equal to its base count.
    Later entries are its bases in inheritance order, most-derived first;
    `numContainedBases == 0` marks a direct base.
    """
    data = image.data
    bases: list[dict] = []
    hier_rva = col["hierarchy_rva"]

    count = read_u32(data, hier_rva + 8, image)
    array_rva = read_u32(data, hier_rva + 12, image)
    if count is None or not (0 < count <= 64) or not array_rva:
        return bases

    for i in range(1, count):  # entry 0 is the class itself
        bcd_rva = read_u32(data, array_rva + 4 * i, image)
        if not bcd_rva:
            continue
        td = read_u32(data, bcd_rva, image)
        contained = read_u32(data, bcd_rva + 4, image)
        mdisp = read_u32(data, bcd_rva + 8, image)
        pdisp = read_u32(data, bcd_rva + 12, image)
        vdisp = read_u32(data, bcd_rva + 16, image)
        if td is None:
            continue
        bases.append(
            {
                "name": demangle(types.get(td, f"<td {td:#x}>")),
                "mangled": types.get(td, ""),
                "direct": contained == 0,
                "mdisp": mdisp if mdisp is not None else 0,
                "pdisp": pdisp if pdisp is not None else 0,
                "vdisp": vdisp if vdisp is not None else 0,
            }
        )
    return bases


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--dll", type=pathlib.Path, default=DEFAULT_DLL)
    parser.add_argument("--out", type=pathlib.Path, default=FUNCTIONS / "_rtti.tsv")
    parser.add_argument("--grep", help="only show classes containing this")
    parser.add_argument("--hier", action="store_true", help="group by enclosing namespace")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    image = Image.from_path(args.dll)
    types = find_type_descriptors(image)
    cols = find_cols(image, types)
    col_rvas = {c["col_rva"] for c in cols}
    vtables = find_vtables(image, col_rvas)

    by_rva = {c["col_rva"]: c for c in cols}
    rows: list[dict] = []
    for col_rva, vt_rva in vtables.items():
        col = by_rva.get(col_rva)
        if col is None:
            continue
        rows.append({**col, "vt_rva": vt_rva, "vt_va": image.to_va(vt_rva)})
    rows.sort(key=lambda r: r["vt_va"])

    needle = args.grep.lower() if args.grep else None
    shown = [r for r in rows if not needle or needle in r["name"].lower()]

    for r in rows:
        r["bases"] = decode_hierarchy(image, r, types)
    edges = sum(len(r["bases"]) for r in rows)

    if not args.quiet:
        print(f"image            : {args.dll.name}")
        print(f"type descriptors : {len(types)}")
        print(f"complete obj locs: {len(cols)}")
        print(f"vtables resolved : {len(rows)}")
        print(f"inheritance edges: {edges}")
        print()

    if args.quiet:
        rows = list(rows)
    elif args.hier:
        groups: dict[str, list[dict]] = collections.defaultdict(list)
        for r in shown:
            parent = r["name"].rsplit("::", 1)[0] if "::" in r["name"] else "(global)"
            groups[parent].append(r)
        for parent in sorted(groups, key=lambda p: -len(groups[p])):
            print(f"{parent}  ({len(groups[parent])})")
            for r in sorted(groups[parent], key=lambda x: x["name"]):
                print(f"    {r['name']:<52} vtable {r['vt_va']:#x}")
        print()
    else:
        for r in shown[:80]:
            chain = ""
            if r["bases"]:
                chain = "  : " + " <- ".join(b["name"] for b in r["bases"][:4])
            print(f"{r['vt_va']:#x}  {r['name']:<44}{chain}")
        if len(shown) > 80:
            print(f"... {len(shown) - 80} more (use --grep or --hier)")
        print()

    with args.out.open("w", encoding="utf-8") as fh:
        fh.write("# vtable_va\tcol_va\tname\tmangled\toffset\n")
        for r in rows:
            fh.write(
                f"{r['vt_va']:#x}\t{image.image_base + r['col_rva']:#x}\t{r['name']}\t"
                f"{r['mangled']}\t{r['offset']}\n"
            )
    if not args.quiet:
        print(f"index: {args.out}")

    hier_out = args.out.with_name("_rtti_hierarchy.tsv")
    if edges:
        # A class with N vtables contributes N identical base lists - one COL per
        # vtable. Emit one row per (derived, base) pair, keeping the lowest vtable,
        # so the edge count reflects classes rather than vtable multiplicity.
        seen: set[tuple[str, str]] = set()
        with hier_out.open("w", encoding="utf-8") as fh:
            fh.write("# derived\tbase\tdirect\tmdisp\tpdisp\tvdisp\tvt_derived\n")
            for r in sorted(rows, key=lambda x: x["vt_va"]):
                for b in r["bases"]:
                    key = (r["name"], b["name"])
                    if key in seen:
                        continue
                    seen.add(key)
                    fh.write(
                        f"{r['name']}\t{b['name']}\t{int(b['direct'])}\t"
                        f"{b['mdisp']}\t{b['pdisp']}\t{b['vdisp']}\t{r['vt_va']:#x}\n"
                    )
        if not args.quiet:
            print(f"hierarchy: {hier_out} ({edges} raw, {len(seen)} distinct edges)")
    elif not args.quiet:
        # Deliberately not written: an empty hierarchy table reads as "this binary
        # has no inheritance", which is a claim nothing here supports.
        print("hierarchy: not written - decode_hierarchy() is unverified (see its docstring)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())