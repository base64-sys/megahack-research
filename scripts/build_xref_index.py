"""Build a string -> referencing-function index over the recovered tree.

Answering "which code touches 'Best Run'?" across 18,800 files is the practical
bottleneck for changing anything, and `tools/pe_xrefs.py` only reports xrefs to a
handful of strings at a time. This builds the whole table once.

How it works, and why it needs no disassembler:

1. Every printable ASCII and UTF-16LE run in the image is located as a (VA, text)
   pair. Offsets come from `scripts/pe_rva.py`, which refuses zero-fill and gap
   addresses rather than silently reading a later section.
2. The recovered Ghidra output already names every rip-relative data reference in
   the form `DAT_180575a00`. Scanning those files for `DAT_<hex>` therefore
   recovers the string -> function relation without disassembling anything.
3. `recovered/functions/_wrappers.tsv` is folded in, so a reference made from
   inside a hook-wrapper thunk is attributed to the real hook target.

    python scripts/build_xref_index.py
    python scripts/build_xref_index.py --grep best      # then read the output

Output: recovered/functions/_xrefs.tsv, gitignored with the rest of recovered/.
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from pe_rva import Image  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parent.parent
BASELINE = REPO / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"
FUNCTIONS = REPO / "recovered" / "functions"

ASCII_RE = re.compile(rb"[\x20-\x7e]{4,}")
# Ghidra's symbol for a rip-relative reference into data.
DAT_RE = re.compile(r"\b_?DAT_([0-9a-f]{8,16})\b")
# A string Ghidra decoded successfully is inlined as a literal rather than named,
# so most user-visible strings never appear as DAT_ symbols. Match those too.
LITERAL_RE = re.compile(r'"((?:[^"\\\n]|\\.){4,})"')


def collect_strings(image: Image, data: bytes) -> dict[int, str]:
    """Map VA -> string text for every printable run Ghidra could have named."""
    found: dict[int, str] = {}
    start = 0
    limit = len(data)
    while start < limit:
        m = ASCII_RE.search(data, start)
        if not m:
            break
        va = image.va_of_file_offset(m.start())
        if va is not None:
            found[va] = m.group().decode("ascii", errors="replace")
        start = m.end()
    return found


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--image", type=pathlib.Path, default=BASELINE)
    parser.add_argument("--functions", type=pathlib.Path, default=FUNCTIONS)
    parser.add_argument("--grep", nargs="*", help="only report strings matching these substrings")
    parser.add_argument("--limit", type=int, default=40, help="rows to print per query")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    if not args.image.exists():
        raise SystemExit(f"image not found: {args.image}")
    if not args.functions.is_dir():
        raise SystemExit(f"no recovered tree at {args.functions}; run scripts/decompile_all.py first")

    image = Image.from_path(args.image)
    data = args.image.read_bytes()
    strings = collect_strings(image, data)

    # VA referenced by each recovered function, via Ghidra's DAT_ symbols and via
    # any string literal Ghidra inlined.
    by_text: dict[str, list[int]] = collections.defaultdict(list)
    for va, text in strings.items():
        by_text[text].append(va)

    refs: dict[int, list[str]] = collections.defaultdict(list)
    files = sorted(args.functions.glob("*.c"))
    dat_refs = 0
    literal_refs = 0
    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        seen = {int(h, 16) for h in DAT_RE.findall(text)}
        dat_refs += len(seen)
        for literal in LITERAL_RE.findall(text):
            if "\\" in literal:
                continue  # escaped literals are not byte-for-byte comparable
            for va in by_text.get(literal, ()):
                seen.add(va)
                literal_refs += 1
        for va in seen:
            refs[va].append(path.stem)

    # Fold in hook-wrapper targets so references made from a thunk are credited
    # to the function that is actually hooked.
    wrapper_target = {}
    wtsv = args.functions / "_wrappers.tsv"
    if wtsv.exists():
        for line in wtsv.read_text(encoding="utf-8").splitlines():
            if line.startswith("#"):
                continue
            parts = line.split("\t")
            if len(parts) >= 5 and parts[4] != "-":
                base = parts[4].split("+")[0]
                if base:
                    wrapper_target[parts[0]] = base

    out = args.functions / "_xrefs.tsv"
    linked = 0
    with out.open("w", encoding="utf-8") as fh:
        fh.write("# string_va\trefs\tfunctions\ttext\n")
        for va, text in sorted(strings.items()):
            fns = refs.get(va)
            if not fns:
                continue
            # Rewrite thunks to their hook targets, de-duplicate.
            mapped = {wrapper_target.get(f, f) for f in fns}
            linked += 1
            fh.write(f"{va:#x}\t{len(fns)}\t{','.join(sorted(mapped))}\t{text}\n")

    if not args.quiet:
        print(f"strings found        : {len(strings)}")
        print(f"strings referenced    : {linked}")
        print(f"  DAT_ references      : {dat_refs}")
        print(f"  literal references  : {literal_refs}")
        print(f"recovered functions   : {len(files)}")
        print(f"hook targets folded in: {len(wrapper_target)}")
        print(f"index                 : {out}")
        print()
        print("A string appears when at least one decompiled function names it, either")
        print("as a DAT_ symbol or as an inlined literal.")

    if args.grep:
        needles = [n.lower() for n in args.grep]
        print()
        for va, count, mapped, text in _search(out, needles, args.limit):
            print(f"  {va}  [{count} refs]  {text[:90]!r}")
            for fn in mapped.split(",")[:8]:
                print(f"        {fn}")
            if mapped.count(",") + 1 > 8:
                print(f"        ... {mapped.count(',') + 1 - 8} more")
    return 0


def _search(index: pathlib.Path, needles: list[str], limit: int):
    for line in index.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        parts = line.split("\t", 3)
        if len(parts) < 4:
            continue
        va, count, mapped, text = parts
        if any(n in text.lower() for n in needles):
            yield va, count, mapped, text
            limit -= 1
            if limit <= 0:
                return


if __name__ == "__main__":
    raise SystemExit(main())